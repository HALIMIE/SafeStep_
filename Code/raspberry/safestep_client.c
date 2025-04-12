#include <arpa/inet.h>
#include <fcntl.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 200
#define CLIENT_ID "SERVER_SQL"
#define SERVER_IP "10.10.141.80"

#define DB_HOST "localhost"
#define DB_USER "root"
#define DB_PASS "569856"
#define DB_NAME "step_safe_database"

void *receive_thread(void *arg);
void error_handling(const char *msg);
int init_db_connection();
void save_to_db(const char *image_path, float wear_percentage, int timestamp);
int send_message(int sock, const char *to, const char *message);

MYSQL *conn = NULL;
int running = 1;

int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in server_addr;
  pthread_t rcv_thread;

  if (argc != 2) {
    exit(1);
  }

  if (!init_db_connection()) {
    error_handling("MySQL 연결 실패");
  }

  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    error_handling("소켓 생성 오류");
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
  server_addr.sin_port = htons(atoi(argv[1]));

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    error_handling("연결 오류");
  }

  char id_packet[BUF_SIZE];
  sprintf(id_packet, "[%s]", CLIENT_ID);
  if (write(sock, id_packet, strlen(id_packet)) == -1) {
    error_handling("ID 전송 오류");
  }

  char response[BUF_SIZE];
  int len = read(sock, response, BUF_SIZE - 1);
  if (len <= 0) {
    error_handling("서버 응답 읽기 오류");
  }
  response[len] = '\0';

  if (strstr(response, "등록되지 않은 ID") || strstr(response, "이미 로그인")) {
    close(sock);
    exit(1);
  }

  pthread_create(&rcv_thread, NULL, receive_thread, (void *)&sock);

  char input[BUF_SIZE];
  char to[BUF_SIZE];
  char message[BUF_SIZE];

  while (running) {
    fgets(input, BUF_SIZE, stdin);
    input[strlen(input) - 1] = '\0';

    if (strcmp(input, "IDLIST") == 0) {
      send_message(sock, "IDLIST", "");
    } else if (strchr(input, ':') != NULL) {
      char *delimiter = strchr(input, ':');
      int to_len = delimiter - input;
      strncpy(to, input, to_len);
      to[to_len] = '\0';
      strcpy(message, delimiter + 1);
      send_message(sock, to, message);
    } else {
      printf("잘못된 명령 형식입니다.\n");
    }
  }

  close(sock);
  if (conn != NULL) {
    mysql_close(conn);
  }

  return 0;
}

int send_message(int sock, const char *to, const char *message) {
  char formatted_msg[BUF_SIZE];
  sprintf(formatted_msg, "[%s]:%s", to, message);

  if (write(sock, formatted_msg, strlen(formatted_msg)) <= 0) {
    return 0;
  }
  return 1;
}

int copy_image_to_web_dir(const char *source_path,
                          const char *destination_path) {
  char command[1024];
  char full_source_path[1024];

  snprintf(
      full_source_path, sizeof(full_source_path), "/mnt/ubuntu_nfs/Pictures/%s",
      strrchr(source_path, '/') ? strrchr(source_path, '/') + 1 : source_path);

  snprintf(command, sizeof(command), "sudo mv %s %s", full_source_path,
           destination_path);

  if (system(command) == 0) {
    return 1;
  } else {
    return 0;
  }
}

void *receive_thread(void *arg) {
  int sock = *((int *)arg);
  char buffer[BUF_SIZE];
  int len;

  while (running) {
    memset(buffer, 0, BUF_SIZE);
    len = read(sock, buffer, BUF_SIZE - 1);

    if (len <= 0) {
      running = 0;
      break;
    }

    buffer[len] = '\0';
    printf("수신: %s", buffer);

    if (strstr(buffer, "image_path:") && strstr(buffer, "wear_percentage:") &&
        strstr(buffer, "timestamp:")) {
      char image_path[BUF_SIZE];
      float wear_percentage;
      int timestamp;

      if (sscanf(buffer,
                 "[JETSON -> SERVER_SQL] image_path:%s wear_percentage:%f "
                 "timestamp:%d",
                 image_path, &wear_percentage, &timestamp) == 3) {
        if (wear_percentage >= 50.0 && wear_percentage < 70.0) {
          send_message(sock, "BOX_ARD", "OPEN_50");
        } else if (wear_percentage >= 70.0) {
          send_message(sock, "BOX_ARD", "OPEN_70");
        }

        char source_image_path[BUF_SIZE];
        snprintf(source_image_path, sizeof(source_image_path),
                 "/mnt/ubuntu_nfs/Pictures/%s", image_path);

        if (copy_image_to_web_dir(source_image_path, "/var/www/html/images/")) {
          save_to_db(image_path, wear_percentage, timestamp);
        }
      } else {
        printf("잘못된 데이터 형식\n");
      }
    }
  }

  return NULL;
}

int init_db_connection() {
  conn = mysql_init(NULL);
  if (conn == NULL) {
    fprintf(stderr, "mysql_init() 실패\n");
    return 0;
  }

  unsigned int timeout = 7;
  mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

  for (int retry = 0; retry < 3; retry++) {
    if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL,
                           0)) {
      return 1;
    }

    fprintf(stderr, "MySQL 연결 실패: %s (시도 %d/3)\n", mysql_error(conn),
            retry + 1);
    sleep(2);
  }

  return 0;
}

void save_to_db(const char *image_path, float wear_percentage, int timestamp) {
  char query[BUF_SIZE * 2];

  if (conn == NULL || mysql_ping(conn) != 0) {
    if (conn != NULL) mysql_close(conn);

    if (!init_db_connection()) {
      return;
    }
  }

  sprintf(query,
          "INSERT INTO shoe_wear_data (timestamp, image_path, wear_percentage) "
          "VALUES (FROM_UNIXTIME(%d), '%s', %.2f)",
          timestamp, image_path, wear_percentage);

  if (mysql_query(conn, query)) {
    fprintf(stderr, "MySQL 쿼리 오류: %s\n", mysql_error(conn));
  } else {
    printf("데이터베이스 저장 성공 (이미지: %s, 마모율: %.2f%%)\n", image_path,
           wear_percentage);
  }
}

void error_handling(const char *msg) {
  perror(msg);
  exit(1);
}
