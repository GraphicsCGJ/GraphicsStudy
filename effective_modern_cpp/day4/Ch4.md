- [Item 26. 보편 참조에 대한 Overload를 피하라](#item-26-보편-참조에-대한-overload를-피하라)
  - [26-1 문제 제시](#26-1-문제-제시)
  - [26-2 개선안](#26-2-개선안)
  - [26-3 int형 argument?](#26-3-int형-argument)
  - [26-4 객체화?](#26-4-객체화)
- [Item 27. 보편 참조에 대한 중복적재 대신 사용할 수 있는 기법들을 알아두라.](#item-27-보편-참조에-대한-중복적재-대신-사용할-수-있는-기법들을-알아두라)
  - [27-1. 중복 적재 포기](#27-1-중복-적재-포기)
  - [27-2. const T\& 매개변수 사용.](#27-2-const-t-매개변수-사용)
  - [27-3. 값 전달 방식의 매개변수를 쓴다.](#27-3-값-전달-방식의-매개변수를-쓴다)
  - [27-4. 꼬래표 배분을 사용한다.](#27-4-꼬래표-배분을-사용한다)
  - [27-5. 보편 참조를 받는 템플릿을 제한한다.](#27-5-보편-참조를-받는-템플릿을-제한한다)
- [Item 28. 참조 축약을 숙지하라](#item-28-참조-축약을-숙지하라)
- [Item 29. 이동 연산이 존재하지 않고, 저렴하지 않고, 적영되지 않는다고 가정하라.](#item-29-이동-연산이-존재하지-않고-저렴하지-않고-적영되지-않는다고-가정하라)
  - [개요](#개요)
  - [std::array 예시](#stdarray-예시)
  - [std::string 예시](#stdstring-예시)
  - [제약사항 정리](#제약사항-정리)
    - [이동연산이 없는 경우](#이동연산이-없는-경우)
    - [이동이 더 빠르지 않다.](#이동이-더-빠르지-않다)
    - [이동을 사용할 수 없는 경우](#이동을-사용할-수-없는-경우)
    - [원본 자체가 왼값](#원본-자체가-왼값)
  - [결론](#결론)
- [Item 30. 완벽 전달이 실패하는 경우들을 잘 알아두라.](#item-30-완벽-전달이-실패하는-경우들을-잘-알아두라)
  - [개요](#개요-1)
  - [경우 1. 중괄호 초기치](#경우-1-중괄호-초기치)
  - [경우 2. NULL, 0 인 경우](#경우-2-null-0-인-경우)
  - [경우 3. static const및 constexpr 자료멤버.](#경우-3-static-const및-constexpr-자료멤버)
  - [경우 4. 중복적재된 함수 이름및 템플릿 이름](#경우-4-중복적재된-함수-이름및-템플릿-이름)
  - [경우 5. 비트필드.](#경우-5-비트필드)
- [람다란?](#람다란)
  - [람다 표현식?](#람다-표현식)
  - [클로저?](#클로저)
  - [갈무리?](#갈무리)
  - [클로저 클래스](#클로저-클래스)
  - [람다 예](#람다-예)
- [Item 31. 기본 갈무리 모드를 피해라.](#item-31-기본-갈무리-모드를-피해라)
- [Item 32. 객체를 클로저 안으로 이동하려면 초기화 갈무리를 사용하라.](#item-32-객체를-클로저-안으로-이동하려면-초기화-갈무리를-사용하라)
- [Item 33. std::forward를 통해 전달할 auto\&\& 매개변수는 decltype을 사용해라.](#item-33-stdforward를-통해-전달할-auto-매개변수는-decltype을-사용해라)
- [Item 34. std::bind보다 람다를 선호해라.](#item-34-stdbind보다-람다를-선호해라)


# Item 26. 보편 참조에 대한 Overload를 피하라
> Overload vs Override
> Overload는 argument 갯수나 타입이 다른 경우
> Override는 하위클래슥 상위클래스 메소드를 재정의하는 경우

## 26-1 문제 제시
```cpp
std::multiset<std::string> names; // 전역 컨테이너.
void logAndAdd(const std::string& name)
{
  auto now = std::chrono::system_clock::now();  // 현재시간
  log(now, "logAnddAdd"); // 로그에 기록
  names.emplace(name); // 컨테이너에 데이터 넣기.
}

// 1. 왼값
std::string petName("Darla");
logAndAdd(petName)

// 2. 오른값
logAndAdd(std::string("Perephone"));

// 3. 문자열 리터럴 (상수)
logAndAdd("Patty Dog");
```

1번의 경우
petname(왼값) => 함수 내 name(왼값참조) => names.emplace(name) 복사되며 대입

2번의 경우
std::string("Perephone")(오른값)
=> 함수 내 name은 왼값참조 이므로, 오른값에 대한 임시객체가 생성
=> 임시객체에 대한 왼값참조 name
=> 컨테이너에 복사.

3번의 경우
"Patty Dog"(문자열 리터럴, 오른값)
=> 하지만 이친구도 왼값참조 뿐이므로 임시객체가 생성되고 해당 객체가 왼값참조됨.
=> 컨테이너에 복사.

2,3번 예제에서 사실 오른값 참조만 잘 써주면 모두 이동만 하다 emplace 단계에서 1회 복사 일어나는 것으로 끝낼 수도 있다.

## 26-2 개선안
아래는 책에서 계속 말하는 완벽 전달이 된다.
보편참조와 forward의 조합으로 이동만 일어난다.
```cpp
std::multiset<std::string> names; // 전역 컨테이너.

void logAndAdd(T&& name)
{
  auto now = std::chrono::system_clock::now();
  log(now, "logAnddAdd");
  names.emplace(std::forward<T>(name));
}
```

## 26-3 int형 argument?
전역 컨테이너는 **std::string**을 보고있지만, 이름을 직접 얻지 못하고, 인덱싱을 통해 문자열을 넘기는 경우를 생각해보고,
아래와 같이 Overload되었다고 생각하자.

```cpp
void logAndAdd(int idx)
{
  auto now = std::chrono::system_clock::now();
  log(now, "logAnddAdd");
  names.emplace(nameFromIdx(idx));
}

// 완벽전달로 이미 해결된 놈들
// 1. 왼값
std::string petName("Darla");
logAndAdd(petName)
// 2. 오른값
logAndAdd(std::string("Perephone"));
// 3. 문자열 리터럴 (상수)
logAndAdd("Patty Dog");

// 문제 케이스: 템플릿으로 연역된 함수가 불릴까? int로 오버로드한게 불릴까?
// 보통함수가 매칭이 될 때는 보통함수가 우선이다. (보통함수 = 템플릿으로 인스턴스화된게 아닌 놈들)
// 4. int형을 넘기는 케이스
logAndAdd(22);

// 5. short형을 넘기는 경우
short nameIdx;
logAndAdd(nameIdx);
```
4번의 경우 매칭되는 오버로드 함수가 존재해서 괜찮지만,
5번의 경우 매칭되는 오버로드 함수가 없다. 이 때 컴파일러는 두 가지 중에 하나를 선택해야 한다.

1번안은 nameIdx를 int로 형변환을 해서 진행하는 것과,
2번안은 template에서 T를 short로 인식해서 함수를 호출하도록 하는 방식이 있다.

컴파일러는 기본적으로 타입에 대한 정확한 부합이 형변환보다 우선시 된다.
따라서 보편참조가 존재하는 경우 대부분 형변환을 피하기 위해 보편참조 함수들이 호출된다.

## 26-4 객체화?
```cpp
class Person {
public:
  template<typename T>
  explicit Person(T&& n) : name(std::forward<T>(n));

  explicit Person(int idx) : name(nameFromIdx(idx));

  Person(const Person& rhs); // compiler
  Person(Person&& rhs); // compiler

private:
  std::string name;
};

Person p("Nancy");
auto cloneOfP(p);
```

**Person p("Nancy");** 로 객체 생성 후, **auto cloneOfP(p);** 수행 시,
p의 타입은 **Person**이기 때문에 컴파일러에 의해 생성된 복사생성자가 불리지 않는다.
왜냐하면 기본 복사 생성자의 정확한 타입은 **const Person&** 이기 때문.

따라서 이런 경우도 완벽전달로 처리하고자 한다. 따라서 의도와 다르게 동작한다.
이를 해결하기 위해선 아래와 같이 변경하면 된다.

```cpp
const Person cp("Nancy");
auto cloneOfP(cp);
```

이 경우 의도한대로 동작하게 된다.
하지만 이렇게 해도 중복적재로 인한 문제가 해결되진 않는데, 또다른 문제로

```c++
class SpecialPerson: public Person {
public:
  SpecialPerson(const SpecialPerson& rhs)
  : Person(rhs)
  { ... }

  SpecialPerson(const SpecialPerson& rhs)
  : Person(std::move(rhs))
  { ... }
};
```
와 같은 경우에, 자연스럽게 기반클래스 생성자를 통해 파생클래스 생성자가 호출될 것 같지만,
기반클래스 생성자에선 **const SpecialPerson** 를 받는 생성자 자체가 없으므로 완벽전달 생성자가 호출되고, 이로인해 에러가 발생하여 컴파일되지 않는다.

# Item 27. 보편 참조에 대한 중복적재 대신 사용할 수 있는 기법들을 알아두라.
Item 26에선 보편참조에 쓰이는 함수를 Overload할때 발생할 수 있는 문제들을 짚어봤다.
그러면 해결 방안은 무엇이 있을까?

## 27-1. 중복 적재 포기
함수 이름을 아예 나누어 구분해버린다.
하지만 이런 문제는 생성자에 대한 보편참조엔 통하지 않는다. (이름이 애초에 정해져있음)
따라서 좋지않다.

## 27-2. const T& 매개변수 사용.
Item. 26에서 잠깐 소개된 const T&매개변수를 사용한 함수를 오버로드 해서 복사생성과 관련된 예외처리를 하는 방법이 있다.
다만 이는 c++98과 같은 옛날 방식이고, 자세한 사항은 책에 나와있진 않지만 효율적이지 않다.
( 아마 modern c++에선 컴파일러를 통해 처리할 수 있기 때문인 것 같다. )

## 27-3. 값 전달 방식의 매개변수를 쓴다.
이동 연산 자체를 포기해서 pass by value를 해버리면 사실 모든게 해결된다.
즉, 아래와 같이 매개변수에 값을 복사해온 뒤, 해당 값을 move를 통해 이동시켜서 객체에 저장해준다.
```cpp
class Person {
public:
  explicit Person(std::string n) : name(std::move(n))
    {}
  explicit Person(int idx) : name(nameFromIdx(idx))
    {}
    //....
};
```
이 경우, 문제는 따로 발생하지 않고, 형식연역도 복잡하게 발생하지 않는다.
short가 오면 int로 형 확장이 되고, string이 오면 매개변수에 값을 복사하고 우측값으로 변환하여 name에 밀어넣는다.

## 27-4. 꼬래표 배분을 사용한다.
컴파일러를 사용하는 해결방법이다.
아래 코드를 보자.

```cpp
template<typename T>
void logAndAdd(T&& name)
{
  logAndAddImpl(
    std::forward<T>(name),
    std::is_integral<typename std::remove_reference<T>::type>()
  );
}
```
**logAndAdd** 자체는 보편참조로 모든 타입에 대해 다 받는다. 이전과 마찬가지로 short나 int. string에 대해 구분없이 받는다.
받은 뒤, **logAndAddImpl** 함수를 호출하는데, 해당 함수의 두 번째 인자는 컴파일러에 의해 지정된 값이다.
**std::is_integral<T>** 은 뒤에 들어오는 T가 정수형일 경우 **true**를 리턴해주고, 정수형이 아닐 경우 **false**를 리턴한다.
**std::remove_reference<T>** 은 T의 타입이 참조일 경우 참조를 지운다. 컴파일러 입장에선 int&와 int는 엄연히 다른 타입이기 때문이다.

그럼 logAndAddImpl의 구현은 어떻게 될까?

```cpp
template<typename T>
void logAndAddImpl(T&& name, std::false_type)
{
  auto now = std::chrono::system_clock::now();
  log(now, "logAnddAdd");
  names.emplace(std::forward<T>(name));
}

std::string nameFromIdx(int idx);
template<typename T>
void logAndAddImpl(int idx, std::true_type)
{
  logAndAdd(nameFromIdx(idx));
}
```
두 번째 매개변수를 보면 **std::false_type**, **std::true_type** 인걸 볼 수 있다.
해당 값들은 **std::is_integral** 에 의해 결정된다.
정수형이 아닐 경우, 문자열이라고 예상하고 위 함수를 탄다.
정수형일 경우, 아래 로직을 탄다.

* 왜 true/false가 아닌가?
우리가 보통의 경우처럼 런타임에 로직을 짠다면, true / false에 따라 함수를 두 개 만들어 물려줫을 텐데
is_integral을 통해 나온 값은 컴파일 타임에 정해지는 값이다.
런타임에서 해당 값을 보고 어떤함수를 탈 지 정해지는게 아니다.
따라서 true/false를 통해 로직을 짤 수가 없고 std::true_type / std::false_type라는 컴파일타임 변수를 통해 로직을 구현한다.
컴파일 타임변수이기 때문에 일부 좋은 컴파일러들은 컴파일이 완료된 후엔 해당 함수들에서 컴파일 변수들에 대해선 따로 공간할당을 하지 않는다고 한다.

이렇게 로직 뒤에 컴파일 변수를 달아 로직을 구분하는 것을 꼬리표 배분이라고 한다.

## 27-5. 보편 참조를 받는 템플릿을 제한한다.
꼬리표 배분 말고 두 번째 컴파일 테크닉이다.
템플릿은 우리가 어떤 틀을 하나 놓고, 들어오는 타입에 따라 함수나 클래스 등을 마구마구 찍어주는 놈이다.
하지만 모든 타입에 하지 않고 특정 타입에 대해서만 우리가 이런 동작을 수행할 수 있도록 한다면,
보편 참조가 일어날 때, 특정 타입에 대해선 보편 참조를 타지 않도록 할 수 있다.

이 때 사용되는 것이 enable_if이다.

```cpp
class Person {
public:
  template<typename T,
  typename = typename std::enable_if<CONDITION>::type>
  explicit Person(T&& n);
};
```
위 예시를 보면, typename T 이후에 템플릿 인자가 하나 더 붙은 것을 볼 수 있다.
**std::enable_if<CONDITION>::type** 에서 **CONDITION** 이 true인 케이스에 대해서만 템플릿을 통한 형틀을 찍어낸다.

그러면 앞의 문제들 중에

```cpp
Person p("Nancy");
auto cloneOfP(p);

const Person cp("Nancy");
auto cloneOfP(cp);
```
와 같이 구분하는 문제에서, p에 대한 타입이 Person 일 때 복사생성자를 안타고 보편참조 로직을 타는 것이 문제였다.
이를 std::is_same 등을 해결하면 쉽게 해결할 수 있다.

```cpp
class Person {
public:
  template<
  typename T,
  typename = typename std::enable_if<
    !std::is_same<
      Person,
      typename std::decay<T>::type
      >
      ::value
    >::type>
  explicit Person(T&& n);
};
```
코드를 보면, CONDITION에 해당하는 부분이
```cpp
!std::is_same<
  Person,
  typename std::decay<T>::type
  >
  ::value
>::type>
```
인데, Person이라는 타입과 템플릿으로 들어오는 T에 대한 decay된 타입이 같은 지를 확인한다.
같지 않은 경우에만 보편참조 형식을 타도록 하는 로직이다.

* decay란?
타입에는 참조여부, const, volatile등 여러 태그들이 붙어 아예 다른타입으로 변경될 수 있다.
decay는 이런것들을 전부 부패시켜 원본타입만 남도록 컴파일러한테 지시한다.


하지만 이 경우에도 모든 문제가 해결되진 않는데, 앞에서 본 예제 중
```c++
class SpecialPerson: public Person {
public:
  SpecialPerson(const SpecialPerson& rhs)
  : Person(rhs)
  { ... }

  SpecialPerson(const SpecialPerson& rhs)
  : Person(std::move(rhs))
  { ... }
};
```
예제에선 rhs로 들어가는 값들이 SpecialPersion 타입이다.
컴파일러는 천재가 아니기 때문에 기반클래스 타입까지 유추해서 is_same동작을 수행하지 않는다.
따라서 이런 경우를 고려해 **std::is_same** 대신 **std::is_base_of** 가 존재한다.
아래와 같이 코드를 만들면 우리가 생각했던 모든 문제가 해결된다.

```cpp
class Person {
public:
  template<
  typename T,
  typename = typename std::enable_if<
    !std::is_base_of<
      Person,
      typename std::decay<T>::type
      >
      ::value
    >::type>
  explicit Person(T&& n);
  explicit Persion(int idx);
  ...
};
```

다만 고려해야될 것은, 사용성 측면의 문제가 있다.
컴파일러에게 명령을 많이 준다는 것은, 컴파일 에러가 감지될 때 엄청나게 많은 line의 에러를 출력하게 된다.
static_assert를 통해 경고메세지를 줄 수 있지만 컴파일러에 따라 에러 라인 수가 크게 변하지 않는 경우도 많다.
이런걸 고려해서 써야한다.

# Item 28. 참조 축약을 숙지하라
참조 축약이란 무엇인가?

컴파일러는 아래와 같은 타입이 추론될 때 참조자체를 축약한다는 의미이다.
```
auto& && => auto&
T& && => T&
```
기본적인 룰은 참조자가 두 개일때, 왼쪽 참조자가 하나라도 있으면 왼쪽참조로 바뀐다.

보편참조를 수행할 때 내부적으로 동작하는 방식이며
std::forward 또한 참조축약의 대표적인 예시이다.


```cpp
template<typename T>
T&& forward(typename remove_reference<T>::type& param)
{
  return static_cast<T&&>(param);
}
```
위와 같은 예시의 함수에서, 왼값이 올 경우 T는 **Widget&** 으로 추론되어,
```cpp
Widget& && forward(typename remove_reference<Widget&>::type& param)
{
  return static_cast<Widget& &&>(param);
}
```
가 되고, 이는 다시 앞서 언급한 축약과 remove_reference에 의해
```cpp
Widget& forward(Widget& param)
{
  return static_cast<Widget&>(param);
}
```
가 된다.

오른값이 올 경우엔 T 자체는 Widget으로 연역될 것이다. (&&는 이미 둘다 붙어있으므로)
```cpp
Widget&& forward(typename remove_reference<Widget&>::type& param)
{
  return static_cast<Widget&&>(param);
}
===>
Widget&& forward(Widget& param)
{
  return static_cast<Widget&&>(param);
}
```
와 같이 추론된다.


이제 다른 예시를 보며 생각해보자.
```cpp
template<typename T>
void func(T&& param);

Widget widgetFactory();
Widget w;

func(w); // 왼값
func(widgetFactory()); // 오른값

auto&& w1 = w; // 왼값
// auto = Widget&, ==> Widget& && w1 => widget& w1
auto&& w2 = widgetFactory(); // 오른값
// auto = Widget, ==> Widget&& ww => widget&& w2
```

형식 연역의 대표적인 예시인 템플릿과 auto 말고도
decltype과 typedef에 대해서도 참조 축약이 가능하다.

decltype은 책에 예시가 따로 없고
typedef는 아래 예시를 보며 마친다.
```cpp
template<typename T>
class Widget {
  public:
    typedef T&& RvalueRefToT;
    ...
}

Widget<int&> w;
// typedef int& && RvalueRefToT;
// => typedef int& RvalueRefToT;
```

# Item 29. 이동 연산이 존재하지 않고, 저렴하지 않고, 적영되지 않는다고 가정하라.
## 개요
챕터의 전반적인 내용은 이동연산이 modern c++에서 아주 유용한건 맞지만, 제약조건을 다 확인해야 하기 때문에 일단 의심부터 하고 들어가라는 내용을 말한다.

이동연산이 허용되려면 이동연산이 가능한 환경이 세팅되어있어야 한다.
예를 들면 제3의법칙, 제5의법칙 등을 통해 이동생성자들을 정의를 해놓아야 한다던가 하는 것들이 필요하다.

modern c++ 이전에 쓰인 코드 등에선 이런 것들이 규격화되어있지 않아 코드를 컴파일러만 바꾼다고 해서 이동연산을 자동으로 수행하지 않을 것이다.

또한 몇 가지 경우는 이동연산보다 복사가 빠른 경우도 있다.

## std::array 예시
이동연산은 말그대로 참조자를 옮겨주는 것이다. 즉 포인터를 옮기는 것과 같은데
std::array의 경우 이런 것이 불가능하다.
```cpp
std::array<Widget, 10000> aw1;
```
와 같은 컨테이너가 있을 때, 해당 내용을 **std::move** 로 옮기면

```cpp
auto aw2 = std::move(aw1);
```

가 포인터만 갈아치우는게 아니라, 안에있는 내용물을 실제로 복사한다 O(n).

## std::string 예시
string의 경우 표준컨테이너이기 때문에 std::move가 잘 동작하지만, **작은 문자열(small string optimization, SSO)** 최적화라는 기술 덕분에
크기가 작은 문자열에 대해선 복사가 이동보다 더 빠르다고 한다.

왜냐면 작은 문자열에 대해서 힙영역을 안쓰기 때문에 포인터만 갈아치우는게 안된다.

## 제약사항 정리
### 이동연산이 없는 경우
이동연산이 없는 클래스들이 이 경우에 포함된다.
### 이동이 더 빠르지 않다.
SSO나 std::array 같이 이동이 더 느릴 수 있다.
### 이동을 사용할 수 없는 경우
이동생성자는 컴파일러가 자동으로 noexcept 처리를 하는데, 이런 처리가 아예 안된 채로 선언되어있다면 사용이 불가능하다.
### 원본 자체가 왼값
아주 드문 케이스로, 오른값만 이동연산의 원본이 될 수 있는 경우.

## 결론
이동연산이 존재하지 않는 경우와, 더 느릴경우, 적용이 불가능 한 경우를 항상 가정하고 구현해라.


# Item 30. 완벽 전달이 실패하는 경우들을 잘 알아두라.
## 개요
완벽 전달은 결국 형식 연역이 올바르게 이루어져야 동작한다.
f라는 함수에 인자를 전달하는 fwd라는 함수가 있을 때 기본 모양새는 아래와 같다.

```cpp
template<typename T>
void fwd(T&& param) {
  f(std::forward<T>(param));
}
```

이 함수의 가변인수 버전은 아래와 같다.
```cpp
template<typename... Ts>
T&& fwd(Ts&&... params) {
  return f(std::forward<Ts>(param)...);
}
```

fwd와 f를 보고있는데, 기억해야 할 것은
동일한 expr에 대해
**fwd(expr)** 과
**f(expr)** 이 하는 일이 다르다면, 완벽 전달은 실패한다.

## 경우 1. 중괄호 초기치
```cpp
void f(const std::vector<int>& v);
f({1, 2, 3});
fwd({1, 2, 3});
```
**f({1,2,3})** 은 vector로 연역이 된다.
**fwd({1,2,3})** 는 vector로도 인식하지 못하고 initializer로도 인식하지 못한다.
따라서 에러가 난다.

중괄호 초기치에 대한 형식 연역은 **비연역 문맥**이라고 해서 컴파일러가 금지하는 문법이다.
이전에 말했던 건데 auto에 대해선 잘 연역이 된다.

따라서 아래와 같이 우회는 가능하다.

```cpp
auto li = {1,2,3} // initializer_list
fwd(li);
```

## 경우 2. NULL, 0 인 경우
NULL, 0을 쓰지 말라고 챕터 18에서 말했다. 왜냐면 싹다 int로 연역되버린다.
nullptr을 써라.

## 경우 3. static const및 constexpr 자료멤버.

```cpp
class Widget {
public:
  static constexpr std::size_t MinVals = 28; // 선언만 존재
  ...
};
...

std::vector<int> widgetData;
widgetData.reserve(Widget::MinVals);
```
**static const**나 **constexpr** 로 표현된 상수표현식은 컴파일 타임에 MinVals가 언급된 모든 곳에 28이라는 값을 배치한다.
타입 특성 상 선언만 있어도 잘만 동작하고, 굳이 정의를 해주지 않아도 괜찮다.
이 때 만약 **MinVals** 의 주소 혹은 참조를 찾으려는 행위를 한다면 이는 에러를 유발한다.

앞서 언급했듯 f와 fwd관계가 아래와 같을 때,
```cpp
template<typename T>
void fwd(T&& param) {
  f(std::forward<T>(param));
}
```

다음과 같은 행위는 링크가 되지 않아 런타임에 에러를 유발한다. (컴파일은 된다)
```cpp
f(widget::MinVals); // OK. 값이 바뀌어서 f(28)이 된다.
fwd(Widget::MinVals); // 오류가 발생한다. 내부에서 참조연산자를 사용하기 때문
```

에러를 없애려면 전역에 정의를 한 줄 추가하면 되긴한다.

```cpp
constexpr std::size_t Widget::MinVals;
```
28이라는 값 자체를 다시 줄 필요는 없다.

## 경우 4. 중복적재된 함수 이름및 템플릿 이름
```cpp
void f(int (*pf)(int)); // 포인터 구문
void f(int pf(int)); // 비포인터 구문
int processVal(int value);
int processVal(int value, int priority);
f(processVal); // OK.
fwd(processVal); // ERROR
```

**f(processVal);** 의 경우, processVal이 매개변수에 따라 중복적재 되었지만, 컴파일러는 f가 인자 1개만 가지는 함수포인터를 매개변수로 한다는 것을 안다.
따라서 이걸 잘 연역해서 정상동작하게 만들어준다.

하지만 **fwd(processVal);** 에서 processVal 자체에는 형식이 따로 없다. processVal 자체는 서로다른 두 함수가 공유하는 이름에 불과하다.
따라서 이를 연역하는 것 자체가 불가능하고, 형식 연역이 실패한다.
이런 경우도 완벽 전달이 불가능한 경우가 된다.

함수템플릿의 경우도 마찬가지다.

```cpp
template<typename T>
T workOnVal(T param)
{ ... }

fwd(workOnVal); // 오류! workOnVal 의 어떤 인스턴스인지?
```
workOnVal의 경우도 템플릿의 이름인 것이지 이 자체거 어떤 특정 타입을 가지거나 하질 않기때문에 연역이 불가능하다.

따라서 이걸 정상화 시키려면 명시적으로 타입을 지정해주면 된다.

```cpp
using ProcessFuncType = int (*)(int);
ProcessFuncType processValPtr = processVal;
fwd(processValPtr); // processValPtr 의 타입이 이제는 존재.
fwd(static_cast<ProcessFuncType>(workOnVal)); // 캐스팅도 가능해져서 템플릿 함수도 사용 가능.
```


## 경우 5. 비트필드.

비트필드란
```cpp
struct {
  int a : 3
  int b : 3
  int c : 2
} // 8비트, 1바이트.
```
와 같이 비트단위로 변수를 선언한 구조체를 의미한다.

```cpp
struct {
  std::uint32_t version:4,
                IHL:4,
                DCSP:6,
                ECN:2,
                totalLength:16;
  ...
} IPv4Header;
```
와 같은 예시에서

```cpp
void f(std::size_t sz);
IPv4Header h;
f(h.totalLength); // OK
fwd(h.totalLength); // Error
```
fwd가 불가능한 이유는 참조 즉, 포인터를 쓸 것인데 포인터의 최소단위는 1바이트이다.
따라서 비트단위 포인터는 애초에 존재가 불가능하다.
```cpp
auto length = static_cast<std::uint16_t>(h.totalLength);
fwd(length);
```
위와 같이 만들면 비트필드 값이 복사된다. 이와같이 복사한 데이터를 fwd에 넘기면 정상동작한다.

# 람다란?
함수 객체를 아주 쉽게 만들어주는 녀석.
sort와 같이 compare 함수등을 짤 때, 굳이 복잡하게 전역에 선언하고 정의하는 등의 귀찮은 일을 안해도 되고,
빠르게 함수 객체를 만들어 사용하기 좋다.
또한 STL 함수들과도 연동하여 유용하게 사용 가능하다.
앞에서 확인했듯 shared_ptr의 삭제자 등도 쉽게 만들 수 있다.
## 람다 표현식?
```cpp
std::find_if(container.begin(), container,end(), [](int val) {return 0 < val && val < 10})
```
와 같이 사용 가능하다.
## 클로저?
람다로 만들어진 함수 객체이다.
## 갈무리?
[] 안에 들어가는 놈으로,
= 이 들어가면 스코프 내의 지역변수를 모두 복사해서 클로저가 사용하고
& 가 들어가면 스코프 내의 지역변수를 모두 참조해서 클로저가 사용한다.
하나하나 쓰고싶으면 [x] 혹은 [&x] 와 같이 사용해서 복사 혹은 참조해서 쓰면 된다.
## 클로저 클래스
클로저를 만드는 데 쓰이는 클래스이다.

## 람다 예
```cpp
{
  int x;
  auto c1  // 클로저 (인스턴스, 함수 객체 같은놈.)
    =
    [x](int y) {return x * y > 55; }; // 클로저 클래스, 함수 클래스같은놈

  auto c2 = c1; // c2는 c1의 복사본 둘다 클로저
  auto c3 = c2 // ..
}
```

# Item 31. 기본 갈무리 모드를 피해라.

```cpp
using FilterContainer =
  std::vector<std::function<bool(int)>>;

FilterContainer filters;
```

에서 람다함수로 필터를 하나 추가한다고 하자.

```cpp
filters.emplace_back(
  [](int value) { return value % 5 == 0; }
);
```
위 예에선 divisor (나누려는놈, 제수) 가 5로 고정인데, 이걸 변경할 필요가 생길 수도 있다.

```cpp
using FilterContainer =
  std::vector<std::function<bool(int)>>;

FilterContainer filters;

void addDivisorFilter() {
  auto calc1 = computeSomeValue1();
  auto calc2 = computeSomeValue2();

  auto divisor = computeDivisor(calc1, calc2);

  filters.emplace_back(
    [&] // 기본 참조 갈무리.
    (int value) {
      return value % divisor == 0;
    }
  );
}
```

이 경우 divisor에 대한 참조를 잃을 수도 있는데, 간단하게 생각해보면 divisor는 **addDivisorFilter** 에 걸려있는 놈이다. 따라서 해당 scope가 벗겨지면 이놈도 사라진다. 이건 UB를 유발한다.

```cpp
filters.emplace_back(
    [&divisor] // 기본 참조 갈무리.
    (int value) {
      return value % divisor == 0;
    }
  );
```
얘도 똑같다. 원본의 scope는 변경되지 않음.

람다가 바깥으로 나갈 일이 없는 경우는 괜찮다.

```cpp
template<typename C>
void workWithContainer(const C&) {
  auto calc1 = computeSomeValue1();
  auto calc2 = computeSomeValue2();

  auto divisor = computeDivisor(calc1, calc2);

  using ContElemT = typename C::value_type;

  using std::begin;
  using std::end;

  if (std::all_of(
    begin(container), end(container),
    [&]
    (const ContElemT& value) {
    // (const auto& value) {  // c++ 14버전
      return value % divisor == 0;
    }
  )) {
    ...
  }
  else {
    ...
  }
}
```

하지만 이 경우에도, 다른 동료가 해당 람다가 정상적으로 필터링 한다고 생각하고 람다 자체만 띠어가서 다른 코드에 심어버릴 수도 있다.

참조를 떼는 것이 방법이 될 순 있다.

```cpp
using FilterContainer =
  std::vector<std::function<bool(int)>>;

FilterContainer filters;

void addDivisorFilter() {
  auto calc1 = computeSomeValue1();
  auto calc2 = computeSomeValue2();

  auto divisor = computeDivisor(calc1, calc2);

  filters.emplace_back(
    [=] // 기본 복사 갈무리.
    (int value) {
      return value % divisor == 0;
    }
  );
}
```

근데 만약 포인터를 넘겨주는 코드를 작성한다면 포인터 삭제 시에 문제가 된다.

다음 객체를 생각해보자.

```cpp
using FilterContainer =
  std::vector<std::function<bool(int)>>;

FilterContainer filters;

class Widget {
public:
  void addFilter() const;
private:
  int divisor;
};

void Widget::addFilter() const {
  filters.emplace_back(
    [=]
    (int value) {
      return value % divisor;
    }
  )
}
```

딱 보면 안전해보이지만 안전하지 않다.

갈무리는 람다가 생성된 범위 안에서 보이는 지역 변수에만 적용이된다. 근데 divisor 자체는 Widget의 멤버이기 때문에, 다른 함수로 갈무리가 될 수가 없다.


```cpp
void Widget::addFilter() const {
  filters.emplace_back(
    [divisor]
    (int value) {
      return value % divisor;
    }
  )
}
```

명시적으로 넣어도 당연히 컴파일되지 않는다.

명시적으로 못잡는 원인은 this가 생략되었기 때문이다.

모든 static이 아닌 멤버변수는 객체에 붙어있고, 이 말은 this포인터에 붙어있단 얘기다.

```cpp
void Widget::addFilter() const {
  filters.emplace_back(
    [=]
    (int value) {
      return value % divisor;
    }
  )
}
```

여기서 갈무리되는 놈은 this가 되고 divisor는 this->divisor가 된다 즉,

```cpp
void Widget::addFilter() const {
  auto currentObjectPtr = this;
  filters.emplace_back(
    [currentObjectPtr]
    (int value) {
      return value % currentObjectPtr->this;
    }
  )
}
```

따라서 이렇게 만들면 객체 수명에 의존하게 될 것이고 의도한대로의 값 복사는 일어나지 않는다.

그러면 스마트 포인터는 안전할까?

```cpp
using FilterContainer =
  std::vector<std::function<bool(int)>>;

FilterContainer filters;

class Widget {
public:
  void addFilter() const;
private:
  int divisor;
};

void Widget::addFilter() const {
  filters.emplace_back(
    [=]
    (int value) {
      return value % divisor;
    }
  )
}


void doSomeWork() {
  auto pw = std::make_unique<Widget>;

  pw->addFillter();
}
```

이렇게 하면, make_unique로 생성된 Widget객체에 대한 스마트 포인터를 통해 필터에 추가한다.

그런데 pw의 수명은 doSomeWork()에 의존된다.

따라서 this객체는 생성 직후 파괴가 된다. 이러면 전역 컨테이너 접근이 UB가 되겠다.

이 문제는 지역적으로 값을 복사해서 넣으면 사실 해결된다.

```cpp
void Widget::addFilter() const {
  auto divisorCopy = divisor;

  filters.emplace_back(
    [divisorCopy] // 기본 값 복사 갈무리
    // [=] // 기본 값 복사 갈무리
    // [divisor = divisor] // c++14 버전. divisor 값을 클로저에 '복사' 하는 것이 보장된다.
    (int value) {
      return value % divisorCopy;
    }
  )
}
```

기본 갈무리를 피해야 하는 다른 이유는 static storage duration에 의존하는 변수들이다.

예를 들면 전역 변수나 static 변수, 함수 등은 람다 안에서 사용할 수 있지만 이 값들은 값 갈무리된 값이 아니다.

진짜 전역 영역을 사용하는 놈들이다.

위 예제를 바꿔서

```cpp
using FilterContainer =
  std::vector<std::function<bool(int)>>;

FilterContainer filters;

void addDivisorFilter() {
  static auto calc1 = computeSomeValue1();
  static auto calc2 = computeSomeValue2();

  static auto divisor = computeDivisor(calc1, calc2);

  filters.emplace_back(
    [=] // 기본 복사 갈무리.
    (int value) {
      return value % divisor == 0;
    }
  );
}
```
를 쓰게 되면 복사 갈무리 되는 값은 아무것도 없다. 그저 스택영역 내에 있는 함수 스코프의 static변수들을 쓰는 것이다.

즉, 자기완결적인 람다가 아니게된다.


# Item 32. 객체를 클로저 안으로 이동하려면 초기화 갈무리를 사용하라.

람다 이동 초기화를 사용하는 방식이 c++ 14랑 c++ 11의 방법이 크게 다르다.

c++11에서의 방법이 구지다고 생각돼서 c++ 14에서 다시 만든거라 생각하면 좋다.

우선 c++ 14에서

```cpp
class Widget {
public:
  ...
  bool isValidate() const ;
  bool isArchived() const ;

private:
  ...
};

auto pw = std::make_unique<Widget>();

auto func =
[pw = std::move(pw)]
() {
  return pw->isValidate() && pw->isArchived();
};

```

와 같이 초기화 갈무리에 이동연산을 더해서 쓸 수 있다.

c++ 11에선 어떨까..?

```cpp

class IsValAndArch {
public:
  using DataType=std::unique_ptr<Widget>;

  explicit IsValAndArch(DataType&& ptr) : pw(std::move(ptr)){}

  bool operator()() const {
    return pw->isValidate() && pw->isArchived();
  }

private:
  DataType pw;
};

auto func = IsValAndArch(std::make_unique<Widget>());
```

일단 길다.

따라서 **std::bind**라는 놈을 이용해 객체를 만들어 사용하면 위와같이 복잡하게 할 필요가 없다.

예를 들어 아래의 c++ 14코드가 있다 치자.

```cpp
std::vector<double> data;

auto func = [data = std::move(data)] {
  ... // data 처리
};
```
아주 쉽다. 이걸 c++ 11로 본다면 어떨까?

```cpp
std::vector<double> data;

auto func = std::bind(
  [] (const std::vector<double>& data) {
    ... // data 처리
  },
  std::move(data)
);

은근 비슷하다.

std::bind를 통해 만들어진 객체는 바인드 객체라고 하는데,

바인드 사용법을 하나 예를 들면

```cpp
// 모두의 코드 참고
void subtract(int x, int y) {
  std::cout << x << " - " << y << " = " << x - y << std::endl;
}

auto subtract_from_2 = std::bind(subtract, std::placeholders::_1, 2);

subtract_from_2(3);  // 3 - 2 를 계산한다.
```

가 있다.

이 경우엔, subtract_from_2는 하나의 객체가 되고, 객체 멤버에 operator()(int placeholder) 같은걸 들고있게 된다.

다시 돌아오면, 이런 느낌으로 람다에 대한 이동 초기화를 할 수 있다.


근데 기본적으로 람다 함수는 const이다. 매개변수로 들어온 변수를 내부에서 변경할 수가 없다.

따라서 mutable을 쓰면 해결이 된다.

```cpp
std::vector<double> data;

auto func = std::bind(
  [] (std::vector<double>& data) mutable {
    ... // data 처리
  },
  std::move(data)
);

```

그럼 다시 아까의 코드로 돌아오면
```cpp
auto pw = std::make_unique<Widget>();

// c++ 14
auto func =
[pw = std::move(pw)]
() {
  return pw->isValidate() && pw->isArchived();
};

auto func11 = std:bind(
  [](const std::unique_ptr<Widget>& pw) { // 왼값이동이 됨.
    return pw->isValidate() && pw->isArchived();
  },
  std::make_unique<Widget>()
);
```
이 되겠다.

# Item 33. std::forward를 통해 전달할 auto&& 매개변수는 decltype을 사용해라.

정말 짧은 챕터라 간단하게 넘어가자.

형식 연역의 대표적인 auto와 template 중 auto는 T라는 타입을 지정하는 놈이 없다.

따라서 auto를 통한 연역된 변수의 타입이 필요할 땐 decltype을 쓰란 얘기다.

```cpp

// 문제상황. 타입을 뭐로쓰지?
auto f = [] (auto&& x) {
  return normalize(std::forward<???>(x));
}

// 아래처럼 쓰면 된다.
auto f = [] (auto&& x) {
  return normalize(std::forward<decltype(x)>(x));
}

```

# Item 34. std::bind보다 람다를 선호해라.

c++ 11에선 거의 대부분

c++ 14에선 거의 항상 람다가 우월하다

책에선 그 예제를 경보 코드를 통해서 알려준다.

```cpp
// 시간상의 한 지점을 대표하는 형식 별칭
using Time = std::chrono::steady_clock::time_point;

// enum class 에 관해서는 항목 10을 조라
enum class Sound { Beep, Siren, Whistle };

// 시간 길이를 나타내는 형식에 대한 별칭 선언
using Duration = std::chrono::steady_clock::duration;

// 시간 t 에서 소리 s 를 기간 d 만큼 출력한다
void setAlarm(Time t, Sound s, Duration d);

// setSoundL은
// 직접 지정해서 한 시간 후부터 30초간 울리게하는 함수 객체이다.
auto setSoundL =
  [](Sound s)
  {
    using namespace std::chrono;

    setAlarm(steady_clock::now() + hours(1),
      s,
      seconds(30));
  };
```

c++14에선 조금 더 간결하게 표현 가능하다.

```cpp
auto setSoundL =
  [](Sound s)
  {
    using namespace std::chrono;
    using namespace std::literals; // 1h, 30s 사용가능

    setAlarm(steady_clock::now() + 1h,
      s,
      30s);
  };
```

이걸 bind를 통해 작성하면 어떻게 될까?

```cpp
using namespace std::chrono;
using namespace std::literals;

using namespace std::placeholders;  // _1 을 사용하는 데 필요함

auto setSoundB =
  std::bind(setAlarm,
    steady_clock::now + 1h,
    _1,
    30s
  );
```

람다 버전에선 **steady_clock::now + 1h** 가 헷갈릴 여지가 없다.

람다함수 setSoundL 내의 setAlarm이 호출되고 1시간 후에 알람이 울릴 것이다.

근데 std::bind의 경우 **steady_clock::now + 1h** 가 std::bind 객체로 전달이 된다.
즉 std::bind 객체의 멤버가 되어버린다. 따라서 std:bind를 호출하고 1시간 후에 알람이 울리게된다.

따라서 setAlarm이 호출되고나서 다시 측정하라고 알려줘야 한다. 이로 인해 std::bind를 두개 더 쓴다.

```cpp
auto setSoundB =
  std::bind(setAlarm,
    std::bind(std::plus<>(), // c++ 14에선 표준 연산자 템플릿은 생략 가능
      std::bind(steady_clock::now),
      1h
    )
    steady_clock::now + 1h,
    _1,
    30s
  );
```

이래도 bind가 좋으면 시력문제라고 함.

여기에 근데 overload가 들어가면 골치아파진다.

```cpp
void setAlarm(Time t, Sound s, Duration d, Volume v);
auto setSoundL =
  [](Sound s)
  {
    using namespace std::chrono;
    using namespace std::literals; // 1h, 30s 사용가능

    setAlarm(steady_clock::now() + 1h,
      s,
      30s);
  };

auto setSoundB =
  std::bind(setAlarm,
    std::bind(std::plus<>(), // c++ 14에선 표준 연산자 템플릿은 생략 가능
      std::bind(steady_clock::now),
      1h
    )
    steady_clock::now + 1h,
    _1,
    30s
  );

```

만약 위와같이 setAlarm이 overload되었다고 치자.

그럼 람다는 알아서 잘 호출을 한다.

그런데 bind에선 overload를 구분할 수가 없다. 즉 컴파일이 되지 않는다.

이런 경우엔 아래와 같이 우회해야 한다.

```cpp
using SetAlarm3ParamType = void(*)(Time t, Sound s, Duration d);

auto setSoundB =
  std::bind(static_cast<SetAlarm3ParamType>(setAlarm),
    std::bind(std::plus<steady_clock::time_point>(),
      std::bind(steady_clock::now),
      1h),
    _1,
    30s);
```

상당히 혐오스럽다.

또한 이런 것 말고도 차이가 하나 더 있는데, 보통의 경우 c++ 컴파일러는 보통함수에 대해 인라인화(#define) 해준다.

근데 std::bind의 경우는 함수 포인터를 들고가는 것이고 컴파일러는 함수포인터에 대해선 인라인할 가능성이 낮아진다.

따라서 성능 상에 손해가 있을 수 있다.

추가로, std::bind와 람다는 인수에서도 차이가 난다.

std::bind는 람다 없이 쓰면 그 자체로 코드를 삽입할 수가 없어 std 함수들을 써서 표현을 해야한다.

```cpp
auto betweenL =
  [lowVal, highVal]
  (const auto& val) {
    return lowVal <= val && val <= highVal;
  };

auto betweenB =
  std::bind(std::logical_and<>(),
    std::bind(std::less_equal<>(), lowVal, _1),
    std::bind(std::less_equal<>(), _1, highVal),);
```

c++11은 표준 연산자 템플릿 생략이 안돼서 더 끔찍하다.

마지막으로, 값인지 참조인지 알기가 어렵다.

```cpp
enum class CompLevel { Low, Normal, High };

// 압축 복사본을 만드는 함수 compress
Widget compress(const Widget& w, CompLevel lev);
Widget w; // 원본

using namespace placeholders;
auto compressRateB = std::bind(compress, w, _1); // 문제 1. w는 값일까 참조일까?

auto compressRateL = [w] (CompLevel lev) {
  return compress(w, lev);
};

compressRateL(CompLevel::High); // 매개변수가 값으로 전달된다.
compressRateB(CompLevel::High); // 문제 2. 매개변수가 어떻게 전달될까?
```


* **문제 1**의 해답은 값으로 전달된다. 바인드 객체에 값이 복사되어 저장되어있다.
* **문제 2**의 해답은 참조로 전달된다. 바인드 객체에 참조값이 전달되어 사용된다.
이렇게 보내야 객체의 함수 호출 연산에서 완벽 전달을 수행할 수 있기 때문이다.

즉 이런 특징들을 종합했을 때 c++ 14에선 bind를 쓸 일이 아예 없다고 봐도 된다.
c++ 11에선 아래 두 가지 경우에 대해선 쓸 수 있다.

* 이동 갈무리
c++ 람다는 이동갈무리를 지원하지 않아서 이 때는 std::bind를 써야한다.

* 다형적 함수 객체
바인드 객체에 대한 함수 호출로 넘어가는 인자들은 완벽 전달로 넘어가기 때문에, 아무 형식이나 다 받을 수 있다.
이는 객체의 템플릿화된 함수 호출 연산자와 묶으려고 할 때 유용하다 (아래에 이어 설명한다)

객체의 템플릿화된 함수 호출 연산자와 묶는다가 무슨 뜻일까?

```cpp
class PolyWidget {
public:
  template<typename T>
  void operator()(const T& param) const;
  ...

};

PolyWidget pw;
auto boundPW = std::bind(pw, _1); // 객체의 템플릿화된 함수가 bind객체의 operator()가 된다.

boundPW(1930);          // int로 연역
boundPW(nullptr);       // nullptr로 연역
boundPW("Rosebud");     // 문자열로 연역

auto boundPW_lambda = // c++ 14에선 람다를 통해 간단히 가능
  [pw](const auto& param) {
    pw(param);
  };
```

전반적으로 c++14에서 가장 유용한 것은 인수 및 리턴에 auto를 넣을 수 있는게 정말 유용한 것 같다.