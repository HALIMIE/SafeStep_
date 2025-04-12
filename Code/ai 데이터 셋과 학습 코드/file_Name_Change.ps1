# 경로 정확히 확인하고 설정
Set-Location "C:\workspace\Left"

# 카운터 초기화
$counter = 1

# 파일 이름 변경
Get-ChildItem -File | Sort-Object Name | ForEach-Object {
    $newName = "Left_$counter$($_.Extension)"
    Rename-Item $_.FullName -NewName $newName
    $counter++
}

# 확장자를 .png로 변경 (필요한 경우)
Get-ChildItem -File | ForEach-Object {
    Rename-Item $_.FullName -NewName "$($_.BaseName).png"
}
