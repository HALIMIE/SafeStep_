#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 200
#define MAX_CLNT 10
#define ID_SIZE 20
#define ARR_CNT 5
#define LOG_BUF_SIZE 256
#define IP_SIZE 20

typedef struct {
  int fd;
  char *from;
  char *to;
  char *msg;
  int len;
} MSG_INFO;

typedef struct {
  int index;
  int fd;
  char ip[IP_SIZE];
  char id[ID_SIZE];
} CLIENT_INFO;

void *clnt_connection(void *arg);
void send_msg(MSG_INFO *msg_info, CLIENT_INFO *first_client_info);
void error_handling(const char *msg);
void log_message(const char *msgstr);

int clnt_cnt = 0;
pthread_mutex_t mutx;

int main(int argc, char *argv[]) {
  int serv_sock, clnt_sock;
  struct sockaddr_in serv_adr, clnt_adr;
  socklen_t clnt_adr_sz;
  int sock_option = 1;
  pthread_t t_id[MAX_CLNT] = {0};
  int str_len = 0;
  int i;
  char client_id[ID_SIZE + 3];
  char *pArray[ARR_CNT] = {0};
  char msg[BUF_SIZE];

  CLIENT_INFO client_info[MAX_CLNT] = {
      {0, -1, "", "SERVER_SQL"}, {0, -1, "", "JETSON"},
      {0, -1, "", "SERVER_LIN"}, {0, -1, "", "SERVER_AND"},
      {0, -1, "", "SENSOR_ARD"}, {0, -1, "", "BOX_ARD"},
      {0, -1, "", "HX_ARD"}};

  if (argc != 2) {
    exit(1);
  }

  if (pthread_mutex_init(&mutx, NULL)) error_handling("뮤텍스 초기화 오류");

  serv_sock = socket(PF_INET, SOCK_STREAM, 0);
  if (serv_sock == -1) error_handling("소켓 생성 오류");

  if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&sock_option,
                 sizeof(sock_option)) == -1)
    error_handling("소켓 옵션 설정 오류");

  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_adr.sin_port = htons(atoi(argv[1]));

  if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    error_handling("바인드 오류");

  if (listen(serv_sock, 5) == -1) error_handling("리슨 오류");

  while (1) {
    clnt_adr_sz = sizeof(clnt_adr);
    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

    if (clnt_cnt >= MAX_CLNT) {
      shutdown(clnt_sock, SHUT_WR);
      continue;
    } else if (clnt_sock < 0) {
      perror("accept()");
      continue;
    }

    str_len = read(clnt_sock, client_id, sizeof(client_id) - 1);
    if (str_len <= 0) {
      shutdown(clnt_sock, SHUT_WR);
      continue;
    }

    client_id[str_len] = '\0';

    client_id[str_len - 1] = '\0';
    char *extracted_id = client_id + 1;
    sprintf(msg, "수신 ID: %s\n", extracted_id);

    for (i = 0; i < MAX_CLNT; i++) {
      if (strcmp(client_info[i].id, extracted_id) == 0) {
        if (client_info[i].fd != -1) {
          sprintf(msg, "[%s] 이미 로그인되어 있습니다!\n", extracted_id);
          write(clnt_sock, msg, strlen(msg));
          log_message(msg);
          shutdown(clnt_sock, SHUT_WR);
          client_info[i].fd = -1;
          break;
        }

        strcpy(client_info[i].ip, inet_ntoa(clnt_adr.sin_addr));

        pthread_mutex_lock(&mutx);
        client_info[i].index = i;
        client_info[i].fd = clnt_sock;
        clnt_cnt++;
        pthread_mutex_unlock(&mutx);

        sprintf(msg, "[%s] 새로운 연결 성공! (IP:%s, FD:%d, 접속자:%d)\n",
                extracted_id, inet_ntoa(clnt_adr.sin_addr), clnt_sock,
                clnt_cnt);
        log_message(msg);
        write(clnt_sock, msg, strlen(msg));

        pthread_create(&t_id[i], NULL, clnt_connection,
                       (void *)&client_info[i]);
        pthread_detach(t_id[i]);
        break;
      }
    }

    if (i == MAX_CLNT) {
      sprintf(msg, "%s 등록되지 않은 ID입니다!\n", extracted_id);
      write(clnt_sock, msg, strlen(msg));
      log_message(msg);
      shutdown(clnt_sock, SHUT_WR);
    }
  }

  pthread_mutex_destroy(&mutx);
  close(serv_sock);
  return 0;
}

void *clnt_connection(void *arg) {
  CLIENT_INFO *client_info = (CLIENT_INFO *)arg;
  int str_len = 0;
  int index = client_info->index;
  char msg[BUF_SIZE];
  char parsed_msg[BUF_SIZE];
  char to_msg[MAX_CLNT * ID_SIZE + 1];
  int i = 0;
  char *pArray[ARR_CNT] = {0};
  char strBuff[LOG_BUF_SIZE] = {0};
  MSG_INFO msg_info;
  CLIENT_INFO *first_client_info;

  first_client_info = (CLIENT_INFO *)((void *)client_info -
                                      (void *)(sizeof(CLIENT_INFO) * index));

  while (1) {
    memset(msg, 0, sizeof(msg));
    str_len = read(client_info->fd, msg, sizeof(msg) - 1);

    if (str_len <= 0) break;

    msg[str_len] = '\0';

    char *recipient = NULL;
    char *content = NULL;

    if (msg[0] == '[') {
      char *close_bracket = strchr(msg, ']');
      if (close_bracket) {
        *close_bracket = '\0';
        recipient = msg + 1;

        if (close_bracket + 1 < msg + str_len && close_bracket[1] == ':') {
          content = close_bracket + 2;
        } else {
          content = "잘못된 메시지 형식";
        }
      }
    }

    if (!recipient || !content) {
      recipient = "ALLMSG";
      content = msg;
    }

    sprintf(strBuff, "메시지: [%s -> %s] %s", client_info->id, recipient,
            content);
    log_message(strBuff);

    msg_info.fd = client_info->fd;
    msg_info.from = client_info->id;
    msg_info.to = recipient;

    if (strcmp(recipient, "SERVER_SQL") == 0 &&
        strstr(content, "image_path:")) {
      sprintf(to_msg, "[%s -> %s] %s", client_info->id, recipient, content);
    } else {
      sprintf(to_msg, "[%s] %s", client_info->id, content);
    }

    msg_info.msg = to_msg;
    msg_info.len = strlen(to_msg);

    send_msg(&msg_info, first_client_info);
  }

  close(client_info->fd);

  sprintf(strBuff, "연결 종료 ID:%s (IP:%s, FD:%d, 접속자:%d)\n",
          client_info->id, client_info->ip, client_info->fd, clnt_cnt - 1);
  log_message(strBuff);

  pthread_mutex_lock(&mutx);
  clnt_cnt--;
  client_info->fd = -1;
  pthread_mutex_unlock(&mutx);

  return NULL;
}

void send_msg(MSG_INFO *msg_info, CLIENT_INFO *first_client_info) {
  int i = 0;

  if (strcmp(msg_info->to, "ALLMSG") == 0) {
    for (i = 0; i < MAX_CLNT; i++) {
      if ((first_client_info + i)->fd != -1) {
        write((first_client_info + i)->fd, msg_info->msg, msg_info->len);
      }
    }
  } else if (strcmp(msg_info->to, "IDLIST") == 0) {
    char *idlist = (char *)malloc(ID_SIZE * MAX_CLNT);
    if (idlist == NULL) {
      log_message("메모리 할당 오류 (IDLIST)");
      return;
    }

    if (strlen(msg_info->msg) > 0) {
      msg_info->msg[strlen(msg_info->msg) - 1] = '\0';
      strcpy(idlist, msg_info->msg);
    } else {
      strcpy(idlist, "[시스템]");
    }

    for (i = 0; i < MAX_CLNT; i++) {
      if ((first_client_info + i)->fd != -1) {
        strcat(idlist, " ");
        strcat(idlist, (first_client_info + i)->id);
      }
    }
    strcat(idlist, "\n");

    write(msg_info->fd, idlist, strlen(idlist));
    free(idlist);
  } else {
    for (i = 0; i < MAX_CLNT; i++) {
      if ((first_client_info + i)->fd != -1) {
        if (strcmp(msg_info->to, (first_client_info + i)->id) == 0) {
          write((first_client_info + i)->fd, msg_info->msg, msg_info->len);
          break;
        }
      }
    }
  }
}

void error_handling(const char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}

void log_message(const char *msgstr) {
  fputs(msgstr, stdout);
  fputc('\n', stdout);
}