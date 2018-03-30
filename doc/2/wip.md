# 2차 과제 - System Programming Assignment #1-1 (proxy server)

```
학    과 : 컴퓨터 공학과
담당교수 : 황호영
분    반 : 목34
학    번 : 2016722092
성    명 : 정동호
```

---
# 목차
1. Introduction
2. Flow chart
3. Pseudo code
4. 결과화면
5. 결론 및 고찰
---

# 1. Introduction
본 과제에서 다룰 프로그램은 캐싱 기능을 수행하는 프록시 서버이다. 이번 1번째 구현 단계에선
- 표준입력으로부터 URL 입력 받기
- SHA1 알고리즘으로 text URL을 hashed URL로 변환하기
- hashed URL을 이용하여 directory와 file 생성하기

이 3개를 구현한다.

# 2. Flow chart

<figure>
  <img src="/images/9fd97f75f55cc4cf73f334fc8e95d52d.png" alt="">
  <figcaption>그림 2-1. main 함수 flowchart</figcaption>
</figure>

<figure class="bigpicture">
  <img src="/images/b11bc91209329e3458a54ac86a1c6517.png" alt="">
  <figcaption>그림 2-2. main 함수 flowchart</figcaption>
</figure>

<figure>
  <img src="/images/5aa8f6a735c64d3b79a99b4ef7372ae0.png">
  <figcaption>그림 2-3. main 함수 flowchart</figcaption>
</figure>