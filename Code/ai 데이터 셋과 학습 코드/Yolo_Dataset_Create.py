import os
import shutil
import random
import yaml

def create_yolo_dataset(source_dir, output_dir, train_ratio=0.8):
    """
    YOLOv8 형식의 데이터셋 구조를 생성합니다.
    계층적 샘플링을 통해 각 클래스가 학습 및 검증 세트에 균형있게 분포되도록 합니다.

    Args:
        source_dir (str): 원본 이미지와 레이블 파일이 있는 디렉토리
        output_dir (str): 생성될 YOLOv8 데이터셋 디렉토리
        train_ratio (float): 학습 데이터셋의 비율 (0.0 ~ 1.0)
    """
    # 클래스 파일 찾기
    classes_file = os.path.join(source_dir, "classes.txt")

    if not os.path.exists(classes_file):
        print(source_dir)
        print("classes.txt 파일을 찾을 수 없습니다.")
        return

    # 클래스 목록 읽기
    with open(classes_file, 'r', encoding='utf-8') as f:
        classes = [line.strip() for line in f.readlines() if line.strip()]

    print(f"감지된 클래스: {classes}")

    # 출력 디렉토리 생성
    os.makedirs(output_dir, exist_ok=True)

    # train, val 디렉토리 및 하위 디렉토리 생성
    train_img_dir = os.path.join(output_dir, "train", "images")
    train_label_dir = os.path.join(output_dir, "train", "labels")
    val_img_dir = os.path.join(output_dir, "val", "images")
    val_label_dir = os.path.join(output_dir, "val", "labels")

    os.makedirs(train_img_dir, exist_ok=True)
    os.makedirs(train_label_dir, exist_ok=True)
    os.makedirs(val_img_dir, exist_ok=True)
    os.makedirs(val_label_dir, exist_ok=True)

    # 이미지 파일 목록 가져오기
    image_extensions = ['.jpg', '.jpeg', '.png', '.bmp']
    image_files = []

    for file in os.listdir(source_dir):
        file_path = os.path.join(source_dir, file)
        if os.path.isfile(file_path) and any(file.lower().endswith(ext) for ext in image_extensions):
            # 해당 이미지에 대한 레이블 파일이 있는지 확인
            base_name = os.path.splitext(file)[0]
            label_file = os.path.join(source_dir, base_name + ".txt")

            if os.path.exists(label_file):
                image_files.append(file)

    # 클래스별 이미지 분류
    class_images = {}

    for img_file in image_files:
        base_name = os.path.splitext(img_file)[0]
        label_file = os.path.join(source_dir, base_name + ".txt")

        # 이미지의 주요 클래스 결정 (첫 번째 객체의 클래스 ID 사용)
        with open(label_file, 'r') as f:
            lines = f.readlines()
            if not lines:
                continue  # 레이블이 없는 파일 건너뜀

            # 첫 번째 객체의 클래스 ID 추출
            first_line = lines[0].strip()
            if first_line:
                class_id = int(first_line.split()[0])
                if class_id not in class_images:
                    class_images[class_id] = []
                class_images[class_id].append(img_file)

    # 클래스 분포 정보 출력
    print("클래스별 이미지 수:")
    for class_id, imgs in class_images.items():
        class_name = classes[class_id] if class_id < len(classes) else f"알 수 없는 클래스 {class_id}"
        print(f"  - {class_name}: {len(imgs)}개")

    # 각 클래스별로 동일한 비율로 학습/검증 데이터 분할
    train_files = []
    val_files = []

    for class_id, imgs in class_images.items():
        random.shuffle(imgs)  # 클래스 내에서 이미지 섞기
        split_idx = int(len(imgs) * train_ratio)
        train_files.extend(imgs[:split_idx])
        val_files.extend(imgs[split_idx:])

    # 최종 분할된 데이터 섞기 (같은 클래스가 연속으로 오는 것 방지)
    random.shuffle(train_files)
    random.shuffle(val_files)

    # 파일 복사 함수
    def copy_files(file_list, img_dst_dir, label_dst_dir):
        for img_file in file_list:
            base_name = os.path.splitext(img_file)[0]
            label_file = base_name + ".txt"

            # 이미지 파일 복사
            shutil.copy2(
                os.path.join(source_dir, img_file),
                os.path.join(img_dst_dir, img_file)
            )

            # 레이블 파일 복사
            shutil.copy2(
                os.path.join(source_dir, label_file),
                os.path.join(label_dst_dir, label_file)
            )

    # 학습 및 검증 파일 복사
    copy_files(train_files, train_img_dir, train_label_dir)
    copy_files(val_files, val_img_dir, val_label_dir)

    # 클래스별 학습/검증 분포 확인
    train_distribution = {class_id: 0 for class_id in class_images.keys()}
    val_distribution = {class_id: 0 for class_id in class_images.keys()}

    # 학습 데이터셋 분포 계산
    for img_file in train_files:
        base_name = os.path.splitext(img_file)[0]
        label_file = os.path.join(source_dir, base_name + ".txt")
        with open(label_file, 'r') as f:
            first_line = f.readline().strip()
            if first_line:
                class_id = int(first_line.split()[0])
                train_distribution[class_id] += 1

    # 검증 데이터셋 분포 계산
    for img_file in val_files:
        base_name = os.path.splitext(img_file)[0]
        label_file = os.path.join(source_dir, base_name + ".txt")
        with open(label_file, 'r') as f:
            first_line = f.readline().strip()
            if first_line:
                class_id = int(first_line.split()[0])
                val_distribution[class_id] += 1

    # 분포 정보 출력
    print("\n클래스별 학습/검증 이미지 분포:")
    for class_id in class_images.keys():
        class_name = classes[class_id] if class_id < len(classes) else f"알 수 없는 클래스 {class_id}"
        train_count = train_distribution[class_id]
        val_count = val_distribution[class_id]
        total = train_count + val_count
        train_percent = (train_count / total) * 100 if total > 0 else 0
        val_percent = (val_count / total) * 100 if total > 0 else 0
        print(f"  - {class_name}: 학습 {train_count}개 ({train_percent:.1f}%), 검증 {val_count}개 ({val_percent:.1f}%)")

    # 데이터셋 정보 YAML 파일 생성
    yaml_content = {
        'path': os.path.abspath(output_dir),  # 데이터셋의 절대 경로
        'train': 'train/images',  # 학습 이미지 경로
        'val': 'val/images',      # 검증 이미지 경로
        'names': {i: name for i, name in enumerate(classes)}  # 클래스 이름
    }

    # YAML 파일 저장
    with open(os.path.join(output_dir, 'data.yaml'), 'w') as f:
        yaml.dump(yaml_content, f, default_flow_style=False, sort_keys=False)

    print(f"\n데이터셋 생성 완료: {output_dir}")
    print(f"학습 이미지: {len(train_files)}개, 검증 이미지: {len(val_files)}개")
    print(f"data.yaml 파일이 생성되었습니다.")

if __name__ == "__main__":
    # 사용 예시
    source_directory = "./pictures"  # 원본 파일이 있는 폴더
    output_directory = "./yolov8_dataset"   # 생성될 YOLOv8 데이터셋 폴더

    # 현재 작업 디렉토리 출력
    print(f"현재 작업 디렉토리: {os.getcwd()}")

    # ./output 디렉토리가 존재하는지 확인
    if not os.path.exists(source_directory):
        print(f"오류: '{source_directory}' 디렉토리가 존재하지 않습니다.")
        # 절대 경로로 변환하여 출력
        abs_path = os.path.abspath(source_directory)
        print(f"절대 경로: {abs_path}")
        exit(1)  # 프로그램 종료
    else:
        print(f"'{source_directory}' 디렉토리가 존재합니다.")
        # 디렉토리 내용 출력
        print(f"디렉토리 내용: {os.listdir(source_directory)}")

    create_yolo_dataset(source_directory, output_directory, train_ratio=0.9)
