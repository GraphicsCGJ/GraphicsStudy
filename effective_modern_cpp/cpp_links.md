
- [개요](#개요)
- [생성자](#생성자)
  - [기본 복사 생성자, 기본 대입 생성자](#기본-복사-생성자-기본-대입-생성자)
  - [이동생성자, 이동 대입 연산자](#이동생성자-이동-대입-연산자)
  - [변환생성자와 explicit](#변환생성자와-explicit)
- [생성자에 사용되는 cpp 세부 개념들 ( cpp 세부 문법? )](#생성자에-사용되는-cpp-세부-개념들--cpp-세부-문법-)
  - [default, delete](#default-delete)
  - [lvalue rvalue 확실하게](#lvalue-rvalue-확실하게)
  - [상수참조](#상수참조)
  - [초기화 리스트란?](#초기화-리스트란)
  - [virtual 등등](#virtual-등등)
  - [std::move, forward 등등](#stdmove-forward-등등)
- [스마트포인터](#스마트포인터)
  - [스마트 포인터의 종류](#스마트-포인터의-종류)
  - [기본부터](#기본부터)
  - [스마트 포인터 링크](#스마트-포인터-링크)
- [Allocator란?](#allocator란)
    - [Allocator 설명 링크](#allocator-설명-링크)
- [타입 캐스팅의 종류?](#타입-캐스팅의-종류)
  - [dynamic\_cast, static\_cast](#dynamic_cast-static_cast)
  - [reference\_wrapper?](#reference_wrapper)
  - [reinterpret\_cast](#reinterpret_cast)
  - [Typedef와 Using의 차이](#typedef와-using의-차이)
- [템플릿](#템플릿)
  - [typename, class](#typename-class)
- [람다함수?](#람다함수)
  - [lambda function example](#lambda-function-example)
- [Stream 관련 모음](#stream-관련-모음)
  - [string formatting](#string-formatting)
- [함수 바인딩](#함수-바인딩)
  - [std::bind, std::placeholders](#stdbind-stdplaceholders)
- [컴파일러](#컴파일러)
  - [맹글링과 디맹글링](#맹글링과-디맹글링)



# 개요
CPP 제품을 유지보수하기 시작하면서, 모르는 cpp문법 정리하는 페이지

# 생성자
## 기본 복사 생성자, 기본 대입 생성자
[일반생성자와 복사생성자의 차이?](https://devbull.xyz/c-bogsa-saengseongja-dipolteu-bogsa-saengseongjaran/)
일반생성자보단 복사생성자(참조자를 통한 값의 대입)이 무조건 좋을듯.
스택프레임이 올라갈 때 stack에 값복사가 일어난다.
## 이동생성자, 이동 대입 연산자
[이동생성자, 이동대입](https://movahws.tistory.com/10)

## 변환생성자와 explicit
[변환생성자와 Explicit](https://psychoria.tistory.com/40)


# 생성자에 사용되는 cpp 세부 개념들 ( cpp 세부 문법? )
## default, delete
[delete와 default](https://woo-dev.tistory.com/100)

## lvalue rvalue 확실하게
[lvalue_rvalue](https://effort4137.tistory.com/entry/Lvalue-Rvalue)
[rvalue참조자가 필요한이유?](https://marmelo12.tistory.com/298)
## 상수참조
[상수?](https://boycoding.tistory.com/156?category=1007180)
[상수클래스 객체 및 멤버함수](https://boycoding.tistory.com/252)
## 초기화 리스트란?
> https://stackoverflow.com/questions/2785612/c-what-does-the-colon-after-a-constructor-mean 참고
> base constructor, initialise list 관련된 사항.

[멤버 초기화리스트란?](https://lovely-embedded.tistory.com/21)
[사용해야하는 이유?](https://lovely-embedded.tistory.com/22)
## virtual 등등
[virtual, override. final](https://blankspace-dev.tistory.com/412)

## std::move, forward 등등
[자세히 나온 블로그](https://jungwoong.tistory.com/53)

# 스마트포인터
## 스마트 포인터의 종류
> C++ 11부터 나온, c++포인터의 종류
> 우리 코드에선 unique_ptr & shared_ptr이 사용되고 있다.

## 기본부터
[RAII, stack/heap, 메모리 구조](https://woo-dev.tistory.com/89?category=882879)

* RAII? Resource Acquisition is initialization.
자원 획득은 초기화 시점에 일어나야된다 라는 뜻. 초기화는 객체의 올바른 자원 획득을 보장해야 한다는 뜻.
상세 내용은 링크 참조.

## 스마트 포인터 링크
[C++ UniquePtr](https://woo-dev.tistory.com/110)
[C++ SharedPtr](https://woo-dev.tistory.com/111)
[C++ WeakPtr](https://woo-dev.tistory.com/113)
[참조카운팅](https://woo-dev.tistory.com/61)

# Allocator란?
### Allocator 설명 링크
[Allocator를 쓰는 이유](https://uncertainty-momo.tistory.com/53)
[malloc과 new의 차이](https://uncertainty-momo.tistory.com/47)

# 타입 캐스팅의 종류?
## dynamic_cast, static_cast
[다이나믹캐스팅, 스태틱캐스팅, 업캐스팅, 다운캐스팅](https://sexycoder.tistory.com/15)
[정적 지역 변수](https://dataonair.or.kr/db-tech-reference/d-lounge/technical-data/?mod=document&uid=235959)

## reference_wrapper?
[reference_wrapper](https://movahws.tistory.com/48)
[decltype과 auto](http://egloos.zum.com/sweeper/v/3148281)

## reinterpret_cast
[reinterpret_cast에 관해](https://blockdmask.tistory.com/242)

## Typedef와 Using의 차이
[typedef와 using의 차이](https://unikys.tistory.com/381)

# 템플릿
## typename, class
[typename과 class의 용법 차이?](https://blog.naver.com/PostView.nhn?blogId=oh-mms&logNo=222030206308)

# 람다함수?
## lambda function example
[람다함수 및 필요성?](https://blockdmask.tistory.com/491)


# Stream 관련 모음
## string formatting
[Stream 없이 formatting하기?](https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf)

# 함수 바인딩
## std::bind, std::placeholders
[bind와 placeholder 사용 예](https://happynotepad.tistory.com/27)
[bind와 std::function 연결 예](https://yhwanp.github.io/2019/09/15/std-function-and-std-bind/)
[멤버함수 bind방법](https://stackoverflow.com/questions/37636373/how-stdbind-works-with-member-functions)


# 컴파일러
## 맹글링과 디맹글링
[맹글링과 디맹글링?](https://iwantadmin.tistory.com/55)
