from ultralytics import YOLO
import torch
from multiprocessing import freeze_support

def main():
    # GPU 사용 확인
    print(f"CUDA 사용 가능: {torch.cuda.is_available()}")
    print(f"GPU 이름: {torch.cuda.get_device_name(0) if torch.cuda.is_available() else 'None'}")

    # 모델 로드 - 명확히 yolov8s.pt 지정
    model = YOLO('yolov8s.pt')

    # 훈련 시작 (GPU 사용)
    # results = model.train(
    #     data='yolov8_dataset/data.yaml',
    #     epochs=100,
    #     imgsz=640,
    #     batch=16,
    #     name='yolov8_custom2',  # 이름 변경
    #     device=0,
    #     amp=False,  # AMP 비활성화
    #     verbose=True
    # )

    results = model.train(
        data='yolov8_dataset/data.yaml',
        epochs=100,
        imgsz=640,
        batch=16,
        name='yolov8_gray',
        device=0,
        amp=False,
    
        # 흑백 이미지에 적합한 설정
        lr0=0.005,            # 초기 학습률 (약간 낮춤)
        lrf=0.001,            # 최종 학습률
        weight_decay=0.0005,  # 가중치 감소
    
        # 데이터 증강 (흑백 이미지에 적합)
        hsv_h=0.0,            # 색조 조정 (흑백에서는 효과 없음)
        hsv_s=0.0,            # 채도 조정 (흑백에서는 효과 없음)
        hsv_v=0.3,            # 명도 조정 (중요!)
    
        # 기하학적 증강 (여전히 유용)
        translate=0.1,        # 이미지 이동
        scale=0.5,            # 이미지 스케일
        fliplr=0.5,           # 좌우 뒤집기
        mosaic=1.0,           # 모자이크 (여전히 유용)
    
    # 기타 유용한 매개변수
        patience=50,          # 조기 종료
        cos_lr=True,          # 코사인 학습률 스케줄러
        warmup_epochs=3,      # 워밍업 에폭
        box=7.5,              # 박스 손실 가중치
        cls=0.5               # 클래스 손실 가중치
    )

if __name__ == '__main__':
    freeze_support()
    main()