import cv2
import os

def convert_to_grayscale(image):
    """이미지를 흑백(그레이스케일)으로 변환합니다."""
    return cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

def process_image(image_path, output_folder):
    """
    지정된 경로의 이미지를 로드하고 흑백으로 변환한 후 결과를 저장합니다.
    """
    # 출력 폴더가 없으면 생성
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    # 이미지 로드
    image = cv2.imread(image_path)
    if image is None:
        print(f"오류: {image_path}를 로드할 수 없습니다.")
        return

    # 이미지를 흑백으로 변환
    result = convert_to_grayscale(image)

    # 결과 저장
    filename = os.path.basename(image_path)
    output_path = os.path.join(output_folder, f"grayscale_{filename}")
    cv2.imwrite(output_path, result)
    print(f"흑백 이미지가 {output_path}에 저장되었습니다.")

    return result

def process_folder(input_folder, output_folder):
    """
    지정된 폴더의 모든 이미지를 흑백으로 변환합니다.
    """
    # 입력 폴더가 존재하는지 확인
    if not os.path.exists(input_folder):
        print(f"오류: 폴더 {input_folder}가 존재하지 않습니다.")
        return

    # 출력 폴더가 없으면 생성
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    # 지원되는 이미지 확장자
    valid_extensions = ['.jpg', '.jpeg', '.png', '.bmp', '.tiff']

    # 폴더 내의 모든 파일을 반복
    processed_count = 0
    for filename in os.listdir(input_folder):
        file_path = os.path.join(input_folder, filename)

        # 파일인지 확인하고 지원되는 확장자인지 확인
        if os.path.isfile(file_path) and any(filename.lower().endswith(ext) for ext in valid_extensions):
            process_image(file_path, output_folder)
            processed_count += 1

    print(f"총 {processed_count}개의 이미지가 흑백으로 변환되었습니다.")

# 사용 예시:
if __name__ == "__main__":
    # 단일 이미지 처리 예시
    # process_image("path/to/your/image.jpg", "output_folder")

    # 폴더 내 모든 이미지 처리 예시
    process_folder("./30Right", "./30Right_gray")
    process_folder("./30Left", "./30Left_gray")
    process_folder("./Right", "./Right_gray")
    process_folder("./Left", "./Left_gray")
