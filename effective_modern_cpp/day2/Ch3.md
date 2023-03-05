# Effective Modern C++ Study

# 목차
#### 7. 객체 생성 시 괄호와 중괄호 구분
#### 8. 0 과 NULL 보다 nullptr 사용
#### 9. typedef 보다 alias declaration (별칭 선언) 사용
#### 10. 범위 없는 enum 보다 범위 있는 enum 선호
#### 11. 비공개 함수보다 삭제된 함수 선호
#### 12. override 선언
#### 13. const_iterator 사용
#### 14. 예외 없는 함수의 경우 noexcept 선언
#### 15. constexpr 사용
#### 16. 스레드에 안전한 const 멤버 함수 구현
#### 17. 특수 멤버 함수들의 자동 작성 조건
---

## Item 7. 객체 생성 시 괄호와 중괄호 구분

 c++ 11 에서의 객체 생성 구문이 다양
```cpp
int x(0);		// 괄호
int y = 0;		// "=
int z{ 0 };		// 중괄호
int w = { 0 }; 	// "=" 과 중괄호
```

이와 같이 여러 가지 초기화 구문으로 인한 혼동을 해결하기 위해, **균일 초기화 (uniform initialization)** 를 도입함. 균일 초기화 구문은 중괄호를 사용하기에, 이 책에선 **중괄호 초기화(braced initialization)** 이라고 표현함.

```cpp
std::vector<int> v{ 1, 3, 5 }; // v 의 초기 값 1, 3, 5
```

중괄호 구문은 non-static data member 의 초기화 및 복사할 수 없는 객체 ( std::atomic ) 의 초기화 시 사용함. 이처럼 위의 초기화 표현식 중 어디서나 사용할 수 있는 것은 중괄호 구문뿐이기에 균일 초기화라 부름.

```cpp
class Widget {
...
private:
	int x{ 0 }; // 가능
	int y = 0;	// 가능
	int z(0);	// 불가능
}

std::atomic<int> ai1{ 0 };	// 가능
std::atomic<int> ai2(0);	// 가능
std::atomic<int> ai3 = 0;	// 불가능
```


### 중괄호 초기화의 특징
#### 암묵적 좁히기 변환 ( narrowing conversion ) 방지
- 괄호나 "=" 를 이용한 초기화는 좁히기 변환을 점검하지 않지만, 중괄호 초기화는 좁히기 변환을 확인하여 보장되지 않을 경우 컴파일러 오류 발생.

```cpp
double x, y, z;
...
int sum1{ x + y + z };	// 불가능, double 에서 int 표현 X 
int sum2(x + y + z);	// 가능, 표현식 값이 int 에 맞게 잘림
int sum3 = x + y + z;	// 가능, sum2 와 동일
```

#### 가장 성가신 구문 해석 ( most vexing parse ) 에서 자유로움.
- 가장 성가신 구문 해석은 "선언으로 해석할 수 있는 것은 항상 선언으로 해석해야 한다" 는 C++ 규칙에서 비롯된 하나의 부작용.

```cpp
Widget w1(10);	// 인수 10 으로 Widget 생성자 호출  
Widget w2();	// 가장 성가신 구문 해석.
				// Widget을 돌려주는 w2 라는 이름의 함수 선언
Widget w3{};	// 인수 없이 Widget 생성자 호출
```

#### 중괄호 초기화의 단점
- 중괄호 초기화와 std::initializer_list, 생성자 오버로드 해소(Overload Resolution) 규칙 사이에서의 문제 발생
- 생성자 중 std::initializer_list 형식의 매개변수를 선언 한다면, 중괄호 초기화 구문은 std::initializer_list 를 받는 오버로드된 버전을 호출함. 또한 복사 생성이나 이동 생성이 일어났을 때도 std::initializer_list 생성자 호출함
```cpp
class Widget {
public:
	Widget(int i, bool b);
	Widget(int i, double d);
	Widget(std::initializer_list<long double> il);
	...
};

Widget w1(10, true);	// 첫번째 생성자 호출
Widget w2{10, true};	// std::initializer_list 생성자 호출 (10, true -> long double 변환)
Widget w3(10, 5.0);		// 두번째 생성자 호출
Widget w4{10, 5.0};		// std::initializer_list 생성자 호출 (10, 5.0 -> long double 변환)
```
```cpp
class Widget {
public:
	Widget(int i, bool b);
	Widget(int i, double d);
	Widget(std::initializer_list<long double> il);
	operator float() const;
	...
};

Widget w5(w4);				// 복사 생성자 호출
Widget w6{w4};				// std::initializer_list 생성자 호출
							// w4 -> float -> lone double 변환
Widget w7(std::move(w4));	// 이동 생성자 호출
Widget w8{std::move(w4)};	// std::initializer_list 생성자 호출, w6과 동일한 변환
```

- 중괄호 초기화시 std::initializer_list 를 받는 생성자에 대응시키려고 하여 std::initializer_list 생성자가 보다 더 적합한 생성자를 호출할 수 없는 문제가 발생함.
- 아래의 코드에서 컴파일러는 처음 두 생성자를 무시하고 std::initializer_list 생성자를 호출함. 하지만 좁히기 변환을 허용하지 않아 컴파일 오류가 발생.
```cpp
class Widget {
public:
	Widget(int i, bool b);
	Widget(int i, double d);
	Widget(std::initializer_list<bool> il);
	...
};

Widget  w{10, 5.0};	// 불가능, 좁히기 변환 필요
```

- 위의 경우처럼, 컴파일러가 보통의 Overload Resolution 규칙으로 호출하는 경우는 중괄호 초기화의 인수 형식들을 std::initializer_list 안의 형식으로 변환하는 방법이 아예 없을 경우.
- 아래의 경우 int와 bool 이 std::string 으로 변환하는 방법이 없어 std::initializer_list 생성자가 아닌 생성자가 호출.

```cpp
class Widget {
public:
	Widget(int i, bool b);
	Widget(int i, double d);
	Widget(std::initializer_list<std::string> il);
	...
};

Widget w1(10, true);	// 첫번째 생성자 호출
Widget w2{10, true};	// 첫번째 생성자 호출
Widget w3(10, 5.0);		// 두번째 생성자 호출
Widget w4{10, 5.0};		// 두번째 생성자 호출
```

- 기본 생성자와 std::initializer_list 생성자를 지원하는 객체에서 빈 중괄호 쌍으로 객체 생성 시 기본 생성자 호출됨.
- 빈 std::initializer_list 로 std::initializer_list 생성자 호출하고 싶은 경우, 빈 중괄호 쌍으로 호출.
```cpp
class Widget {
public:
	Widget();
	Widget(std::initializer_list<int> il);
	...
};

Widget w1;		// 기본 생성자 호출
Widget w2{};	// 기본 생성자 호출
Widget w3();	// Most vexing parse, 함수 선언
Widget w4({});	// std::initializer_list 생성자 호출
Widget w5{{}};	// std::initializer_list 생성자 호출
```

#### 중괄호 초기화 주의사항

1. 클래스를 작성할 때, 오버로드된 std::initializer_list 생성자 존재한다면, 중괄호 초기화 구문 사용시 이 생성자만 적용될 수 있음을 주의해야 함. 그리고 객체를 생성할 때 괄호와 중괄호를 세심하게 선택해야 함. 둘 중 하나를 기본으로 사용하고, 다른 하나는 꼭 필요할 때만 사용하는 방식으로 사용.
```c++
std::vectpr<int> v1(10, 20);	// 비 std::initializer_list 생성자
								// 모든 요소 값이 20인, 요소 10개짜리 vector 생성
std::vectpr<int> v1{10, 20};	// std::initializer_list 생성자
								// 값이 10, 20인 두 요소를 담은 vector 생성
```

2. 템플릿 내부에서 객체를 생성하는 경우 괄호와 중괄호 중 어느 것을 사용할지 판단이 필요함.

```cpp
template<typename T,		// 생성할 객체의 형식
		 typename... Ts>	// 사용할 인수들의 형식들
void doSomeWork(Ts&&... params)
{
	params...으로부터 지역 T 객체 생성 
	// 1. T localObject(std::forward<Ts>(params)...);	// 괄호 사용
	// 2. T localObject{std::forward<Ts>(params)...};	// 중괄호 사용
	...
}

std::vector<int> v;
...
doSomeWork<std::vector<int>>(10, 20);
// 1번 일시, 요소 10개인 std::vector 
// 2번 일시, 요소 2개인 std::vector
```


## Item 8. 0 과 NULL 보다 nullptr 사용

### 0 과 NULL 의 문제
`0` 과 `NULL` 는 포인터 타입이 아님.

포인터만 사용할 수 있는 위치에서 `0` 을 사용할 경우, null pointer 로 해석하기 하지만 기본적으로 `0` 은 int 이지 포인터가 아님.
`NULL` 또한 컴파일러마다 다르지만, 일반적으로 `0` 으로 정의함.
int 타입의 0 이든 long 타입의 0 이든 중요한 건 포인터 타입은 아님.

 
 ### 포인터와 정수 타입에 대한 오버로딩 문제
`0` 과 `NULL` 모두 포인터 타입이 아니기 때문에, 포인터 타입과 정수 타입에 대한 오버로딩 과정에서 문제가 발생함. `0` 과 `NULL` 로 포인터 타입의 파라미터를 가지는 함수를 호출하고 싶어도 호출할 수 없음. 따라서 C++ 98 에서는 포인터 타입과 정수 타입들에 대해서는 오버로딩을 피함. 이는 C++ 11 에서도 여전히 유효함.
```cpp
void f(int);
void f(bool);
void f(void*);

f(0);    		// f(void*)가 아닌 f(int) 호출
f(NULL); 		// 컴파일되지 않거나, 보통 f(int) 호출
```

### nullptr 
C++ 11 에서는 `0` 과 `NULL` 의 문제를 해결한 `nullptr` 이 추가
`nullptr` 의 실제 타입은 `std::nullptr_t` 이고, 모든 타입의 포인터 타입으로 암시적 형변환이 가능. 즉, `nullptr` 는 모든 타입의 포인터.

### nullptr 장점

#### 포인터와 정수 타입에 대한 오버로딩 문제 해결

`0` 과 `NULL` 을 사용할 때 발생하는 포인터 타입과 정수 타입의 오버로딩 문제를 해결 가능.

```cpp
void f(int);
void f(bool);
void f(void*);

f(nullptr);    		// f(void*) 호출
```

#### 코드의 명확성

`nullptr` 사용할 경우 해당 변수가 포인터 타입인지 정수 타입인지 명확하게 알 수 있음.
```cpp
auto result = findRecord();

if (result == 0) {
	...
}

if (result == nullptr) {
	...
}
```
#### 템플릿 구현시 효과적


```cpp
// 이 함수들은 뮤텍스를 잠그고 호출해야 함.
int 	f1(std::shared_ptr<Widget> spw);
double 	f2(std::unique_ptr<Widget> upw);
bool	f3(Widget* pw);

std::mutex f1m, f2m, f3m;

using MuxGuard = std::lock_guard<std::mutex>;
... 
{
	MuxGuard g(f1m); 		// f1용 뮤텍스로 잠금
	auto result = f1(0); 	// 0을 널 포인터로서 f1에 전달
} 							// 뮤텍스 해제

{
	MuxGuard g(f2m); 			// f2용 뮤텍스로 잠금
	auto result = f2(NULL); 	// NULL을 널 포인터로서 f2에 전달
} 								// 뮤텍스 해제

{
	MuxGuard g(f3m); 			// f3용 뮤텍스로 잠금
	auto result = f3(nullptr); 	// nullptr을 널 포인터로서 f3에 전달
} 								// 뮤텍스 해제
```
위 코드에서 `0`과 `NULL`, `nullptr` 을 사용하였지만, 코드가 정상적을 작동.
이런 종류의 코드 중복을 피하기 위해 템플릿을 구현하면 다음과 같음.

```cpp
// C++11
template <typename FuncType, typename MuxType, typename PtrType>
auto lockAndCall(FuncType func, MuxType& mutex, PtrType ptr) -> decltype(func(ptr)) 
{
	using MuxGuard = std::lock_guard<MuxType>;

	MuxGuard g(mutex);
	return func(ptr);
}

// C++14
template <typename FuncType, typename MuxType, typename PtrType>
decltype(auto) lockAndCall(FuncType func, MuxType& mutex, PtrType ptr) 
{
	using MuxGuard = std::lock_guard<MuxType>;

	MuxGuard g(mutex);
	return func(ptr);
}

auto result1 = lockAndCall(f1, f1m, 0);       // 에러
auto result2 = lockAndCall(f2, f2m, NULL);    // 에러
auto result3 = lockAndCall(f3, f3m, nullptr); // 정상 동작
```

템플릿으로 구현한 코드에서 `0`과 `NULL` 사용할 경우 에러가 발생함.
템플릿이 ptr 에 넘겨준  `0`과 `NULL` 을 연역하는 과정에서 PtrType 을 정수 타입으로 연역하기 때문에, 포인터 파라미터를 사용하는 함수 인자로 정수 타입을 넣어주면서 컴파일 에러 발생.

`nullptr` 의 경우 ptr 의 형식은 std::nullptr_t 로 연역되고, 이를 f3 에 전달하면 std::nullptr_t 에서 Widget* 로 암시적 형변환이 일어나 문제없이 작동함.



## Item 9. typedef 보다 alias declaration (별칭 선언) 사용

STL 컨테이너와 스마트 포인터를 사용하면 `std::unique_ptr<std::unordered_map<std::string, std::string>>` 같은 타입을 사용할 가능성이 있음.


### alias declaration

이를 C++ 11 전에는 `typedef` 를 편리하게 사용함. C++ 11 부터는 `typedef` 를 대신할 `alias declaration` 이 추가됨.
```cpp
typedef std::unique_ptr<std::unordered_map<std::string, std::string>> UPtrMapSS;
using UPtrMapSS = std::unique_ptr<std::unordered_map<std::string, std::string>>;
```

#### alias declaration 장점

 `typedef`나  `alias declaration`나 하는 일은 동일함.
 하지만 `alias declaration`이 더 많은 장점을 가지고 있음.

##### 코드의 가독성
```cpp
// FP는 int 하나와 const std::string& 하나를 받는 함수 포인터
typedef void (*FP)(int, const std::string&);
using FP = void (*)(int, const std::string&);
```

##### 템플릿에 사용 가능

`typedef`와 달리  `alias declaration`는 템플릿화 할 수 있음.
템플릿화된 `alias declaration`를 `alias templates (별칭 템플릿)`이라 부름

```cpp
// alias declaration 
template<typename T>
using MyAllocList = std::list<T, MyAlloc<T>>;

MyAllocList<Widget> lw;

// typedef 
template<typename T>
struct MyAllocList {
	typedef std::list<T, MyAlloc<T>> type;
};

MyAllocList<Widget>::type lw;
```

`typedef` 으로 `alias templates` 구현 시 struct 를 이용한 편법을 사용해야 함. 이를 사용해 클래스 템플릿 멤버 변수으로 사용한다면, `typedef` 이름 앞에 `typename` 을 붙여야 함. 
아래의 코드에서 MyAllocList<T>::type 은 템플릿 매개변수(T)에 의존하는 타입이므로 C++ 규칙으로 인해 `typename` 키워드를 붙여야 함. 하지만 `alias declaration` 사용할 경우, `typename`, `::type` 을 붙일 필요가 없음.

```cpp
// alias declaration 
template <typename T>
class Widget {
private:
	MyAllocList<T> list;
}

// typedef 
template <typename T>
class Widget {
private:
	typename MyAllocList<T>::type list;
}
```

### type trait ( 타입 특성 )

TMP ( Template Meta Programming) 에서 템플릿 타입 매개변수를 받아서 적절히 타입을 변경해야 할 경우가 존재. 
예를 들어, 어떤 타입 T 가 주어졌을 때, T 에 담긴 임의의 const 한정사나 참조 한정사 (&, &&) 를 제거해야 하는 경우.

C++ 11 은 이런 종류의 타입 변환을 지원하는 도구를 제공하고, 이를 `type trait` 이라는 하며 `<type_traits>` 헤더 안에 구현되어 있음.

하지만 C++ 11 에서는 `typedef` 로 구현되어 있고, C++ 14 에서는 `alias declaration` 로 구현되어 있음. 
```cpp
// c++ 11
std::remove_const<T>::type			// const T를 T로 변환
std::remove_reference<T>::type		// T&나 T&&을 T로 변환
std::add_lvalue_reference<T>::type	// T를 T&로 변환

// c++14
std::remove_const_t<T>				// const T를 T로 변환
std::remove_reference_t<T>			// T&나 T&&을 T로 변환
std::add_lvalue_reference_t<T>		// T를 T&로 변환

// c++ 11 버전에서 alias declaration 으로 c++ 14 버전 처럼 사용 가능
template <class T>
using remove_const_t = typename remove_const<T>::type;
template <class T>
using remove_reference_t = typename remove_reference<T>::type;
template <class T>
using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;
```




## Item 10. 범위 없는 enum 보다 범위 있는 enum 선호

`enum` 은 범위 없는 열거형 (unscoped enum) 이라고 부름. 일반적인 규칙으로는 한 중괄호 쌍 안에서 어떤 이름을 선언하면 그 이름의 가시성은 해당 중괄호 쌍이 정의하는 범위로 한정되어야 하지만  `enum` 은 이러한 규칙이 적용되지 않음.

```cpp
enum Color { black, white, red };
auto white = false;  // 에러! 범위에 이미 white 선언되어 있음
```

C++ 11 에서는 이에 대응되는 `enum class` 라는 범위있는 열거형 제공함.

```cpp
enum class Color { black, white, red };
auto white = false;

Color c = white; 			// 에러! 범위에 white 가 없음.
Color c = Color::white;		// ok
auto c = Color::white;		// ok
```


### enum class 의 장점

#### namespace 의 오염을 줄여줌.

`enum class` 는 `:: (범위 지정 연산자)` 를 붙여서 사용 가능함.

#### 열거자들에 타입이 강력하게 적용

기존의 `범위없는 enum` 의 경우 암묵적으로 정수 타입을 변환.
하지만 `enum class` 의 경우 암묵적으로 다른 타입으로 변환되진 않음.
```cpp
// 범위없는 enum
enum Color { black, white, red };
std::vector<std::size_t> primeFactors(std::size_t x);
Color c = red;
...
if (c < 14.5) { // Color 와 double 비교
	auto factors = primeFactors(c);
}

// enum class
enum class Color { black, white, red };
Color c = Color::red;
...
if (c < 14.5) { // 에러! Color 와 double 비교할 수 없음
	auto factors = primeFactors(c); // 에러 std::size_t 에 Color 전달 불가능
}

// enum class 를 다른 타입으로 변환하고 싶으면, 캐스팅 필요
enum class Color { black, white, red };
Color c = Color::red;
...
if (static_cast<double>(c) < 14.5) {
	auto factors = primeFactors(static_cast<std::size_t>(c));
}
```

#### 전방 선언 (forward declaration ) 가능

`enum class` 를 사용하면 열거자들을 지정하지 않고 열거형 이름만 미리 선언 가능.
```cpp
enum Color; 		// 에러
enum class Color; 	// ok
```

`enum` 은 왜 전방 선언이 되지 않는지 알기 위해선 `열거형의 바탕 타입(underlying type)` 에 대해서 알아야 함.

`열거형의 바탕 타입(underlying type)` 은 컴파일러가 결정하는 열거형의 타입.
`바탕 타입`은 `enum` 과 `enum class` 모두 지원하며 타입을 명시하지 않고 정의할 경우, 컴파일러가 내부적으로 최적화된 타입을 결정함.

```cpp
// 컴파일러가 바탕 타입을 char 로 결정
enum Color { black, white, red };

// 컴파일러가 바탕 타입을 char 보다 더 큰 정수 형식으로 결정
enum Status {
	good = 0,
	failed = 1,
	imcomplete = 100,
	corrupt = 200,
	indeterminate = 0xFFFFFFFF
};
```

`enum class` 는 기본 바탕 타입 int 를 지원하지만 `enum` 의 경우 기본 바탕 타입이 없음.
따라서 `enum`을 전방 선언할 경우 컴파일러가 바탕 타입을 결정할 수 없어 에러가 발생.

```cpp
enum Color;		  // 에러
enum class Color; // ok
```

`enum` 을 전방 선언하고 싶을 경우 바탕 타입을 직접 명시해줘야 함.
```cpp
enum Color: std::uint8_t;			// enum 전방 선언, 바탕 타입 std::uint8_t

enum class Status;					// 바탕 타입 int
enum class Status: std::uint32_t;	//바탕 타입 std::uint32_t

enum class Status: std::uint32_t {	// 정의할 때도 바탕 타입 지정 가능
	good = 0,
	failed = 1,
	imcomplete = 100,
	corrupt = 200,
	indeterminate = 0xFFFFFFFF
};
```

`enum`을 전방 선언할 수 없으면 컴파일 의존 관계가 늘어나 `enum` 을 사용하는 모든 코드를 다시 컴파일해야 하는 불상사를 막기 위해서는 전방 선언이 꼭 필요함.


#### 범위없는 enum 의 장점

C++ 11 의 std::tuple 안에 있는 필드를 지칭할 때 유용하게 사용할 수 있음.
```cpp
using UserInfo = std::tuple<std::string		// 사용자 이름
							std::string,	// 이메일 주소
							std::size_t>;	// 평판치
UserInfo uInfo;
auto val = std:get<1>(uInfo);	// 필드 1의 값
```

주석에는 tuple의 각 필드가 뜻하는 바가 나와 있지만, 다른 소스 파일에서 위와 같은 코드를 마주친다면 해당 필드가 무엇을 뜻하는지 알기 힘듦. 이런 경우 `enum`을 사용하여 필드 번호를 필드 이름에 연관시키면 그런 것들을 일일이 기억할 필요가 없음.

```cpp
// enum
enum UserInfoFields { uiName, uiEmail, uiReputation };
 
UserInfo uInfo; 
auto val = std::get<uiEmail>(uInfo); // 이메일 필드의 값을 얻음이 명확함
									 // 암묵적 변환
								
// enum class	 
enum class UserInfoFields { uiName, uiEmail, uiReputation };
 
UserInfo uInfo;           
auto val = std::get<static_cast<std::size_t>(UserInfoFields::uiEmail)>(uInfo);
```

`enum class` 로 구현할 수는 있지만 `enum` 으로 구현한 것보다 훨씬 장황함.



## Item 11. 비공개 함수보다 삭제된 함수 선호

코드의 특정 함수를 호출하지 못하게 하는 가장 흔한 방법은 그 함수를 선언하지 않는 것이지만, C++ 에서는 구현하지 않아도 알아서 만들어주는 함수들이 존재.

예를 들어, C++ 에서 클래스를 구현할 경우 따로 선언하지 않아도 특수 멤버 함수를 만들어 줌.
특수 멤버 함수 : 기본 생성자, 소멸자, 복사 생성자, 복사 대입 연산자, 이동 생성자, 이동 대입 연산자

### 정의되지 않은 private 함수를 이용하는 방법

C++ 98 에서는 특정 함수 사용을 방지하기 위해 private 로 함수를 선언하고 정의는 하지 않는 방법을 이용함. 

```cpp
template <class charT, class traits = char_traits<charT> )>
class basic_ios : public ios_base {
public:
	...
private:
	basic_ios(const basic_ios&);            // not defined
	basic_ios& operator=(const basic_ios&); // not defined
```

private 로 생성한 함수는 외부에서 호출할 수 없고, 멤버함수나 friend 함수에서 private 함수를 호출하더라도 정의가 없어 링크에 실패함. 


### deleted function 을 이용하는 방법

C++ 11 에서는 함수 선언 끝에 `= delete` 를 붙여서 사용을 막을 수 있음.

```cpp
template <class charT, class traits = char_traits<charT> )>
class basic_ios : public ios_base {
public:
	basic_ios(const basic_ios&) = delete;
	basic_ios& operator=(const basic_ios&) = delete;
```

`deleted function`은 private 가 아닌 public 으로 선언하는 것이 관례임. 그 이유는 `deleted function`을 외부에서 호출했을 시, C++ 컴파일러가 함수의 접근성을 먼저 확인한 뒤에 삭제 여부를 확인하기 때문. 즉, private 로 `deleted function`를 선언하면 호출한 함수가 private 라서 에러가 발생한 건지 `deleted function`라서 에러가 발생한 건지 혼동을 줄 수 있음.


### deleted function의 장점

#### 에러 확인 시점이 빠름
`deleted function` 을 사용할 경우 삭제된 함수를 사용했다는 에러 메시지를 통해 의도를 명확하게 알 수 있고, 컴파일 과정에서 확인할 수 있음. private 함수를 사용할 경우 링크 시점에 가서야 에러 메시지를 확인할 수 있음.

### 그 어떤 함수도 삭제 가능
정의되지 않은 private 함수는 오직 클래스 멤버 함수에서만 사용할 수 있었음. 하지만 `deleted function`의 경우 어디에서나 사용 가능.

```cpp
// 정수 값을 하나 받고 그 값이 행운의 번호인지의 여부를 돌려주는 비멤버 함수
bool isLucky(int number);

// 인자들이 int가 아니지만 암묵적으로 int로 형변환되어 함수가 호출됨
if (isLucky('a')) ...
if (isLucky(true)) ...
if (isLucky(3.5f)) ...

// 함수 오버로딩을 통해 타입들을 배제할 수 있음
bool isLucky(char) = delete;
bool isLucky(bool) = delete;
bool isLucky(double) = delete; // double을 삭제하면 float도 배제됨

if (isLucky('a'))  // 에러, 삭제된 함수를 호출하려 함.
if (isLucky(true)) // 에러
if (isLucky(3.5f)) // 에러
```


### 원치 않는 템플릿 인스턴스화를 방지
 `deleted function` 은 원치 않는 템플릿 인스턴스화를 방지할 수 있음.
 예를 들어, 내장 포인터들을 다루는 템플릿을 구현할 경우 다른 포인터들과 달리 특별한 처리가 필요한 `void*`이나 `char*` 를 삭제할 수 있음.

```cpp
template <typename T>
void processPointer(T* ptr);

template <>
void processPointer<void>(void*) = delete;
 
template <>
void processPointer<char>(char*) = delete;

// 해당 타입을 배제할 때는 const 키워드, 나아가 volatile 키워드까지 삭제해야 함
template <>
void processPointer<const void>(const void*) = delete;
 
template <>
void processPointer<const char>(const char*) = delete;
```

그리고 클래스 안의 함수 템플릿의 일부 인스턴스화를 방지하려고 할 경우 private 함수로는 한계가 있음. 아래의 코드는 컴파일되지 않는데, 그 이유는 같은 이름의 함수 템플릿 중 일부를 사용하지 못하게 하고 싶다고 접근 지정자를 다르게 선언할 수는 없기 때문. 또한 템플릿 특수화는 반드시 클래스 범위가 아닌 namespace 범위에서 작성해야 함. 

`deleted function`을 사용한다면 원하는 함수 템플릿 인스턴스화를 방지할 수 있음.

```cpp
class Widget {
public:
	template <typename T>
	void processPointer(T* ptr) {...}
    
private:
	template <>
	void processPointer<void>(void*); //에러
};
```

```cpp
class Widget {
public:
  template <typename T>
  void processPointer(T* ptr) {}
  ...
};
 template <> 
 void Widget::processPointer<void>(void*) = delete;
```


## Item 12. 재정의 함수들을 override 선언

객체 지향 프로그래밍에서는 기반 클래스가 정의한 함수를 상속받은 파생 클래스가 오버라이딩하는 경우를 쉽게 볼 수 있음.

C++ 에서는 상속과 재정의 함수를 통해 기반 클래스 포인터로 파생 클래스 객체를 가리키는 동적 다형성을 구현.
```cpp
class Base {
public:
	virtual void doWork();	// 기반 클래스의 가상 함수
};

// 자식 클래스
class Derived : public Base {
public:
	virtual void doWork(); // Base::doWork 재정의
};

// 파생 클래스 객체를 가리키는 기반 클래스 포인터 생성
std::unique_ptr<Base> upb = std::make_unqiue<Derived>();


// 기반 클래스 포인터로 파생 클래스의 overriding 된 doWork를 호출
upb->doWork();
```

### 함수 overriding 조건

1.  기반 클래스 함수가 반드시 가상 함수이어야 함.
2. 기반 함수와 파생 함수의 이름이 반드시 동일해야 함 ( 단, 소멸자는 예외 ).
3. 기반 함수와 파생 함수의 매개변수 타입들이 반드시 동일해야 함.
4. 기반 함수와 파생 함수의 const 성이 반드시 동일해야 함.
5. 기반 함수와 파생 함수의 반환 타입과 예외 명세가 반드시 호환되어야 함. 
6. 멤버 함수들의 참조 한정사(reference qualifier) 들이 반드시 동일해야 함. ( C++ 11 에서 추가됨)

참조 한정사는 해당 객체(*this) 가 lvalue 나 rvalue 일 때만 멤버 함수를 호출할 수 있도록 &, && 키워드를 붙여줌.
```cpp
class Widget {
public:
	void doWork() &;  // lvalue 참조 한정사
	void doWork() &&; // rvalue 참조 한정사
};

Widget makeWidget(); // Widget 타입의 객체를 반환하는 팩토리 함수

Widget w; // Widget 타입 변수

w.doWork();           // lvalue Widget::doWork & 호출
makeWidget().doWork() // rvalue Widget::doWork && 호출
```

위의 overriding 요구 조건들을 이해하고 아래 코드를 보면 overriding 이 실패한 이유를 알 수 있음.

```cpp
class Base {
public:
	virtual void mf1(void) const;
	virtual void mf2(int x);
	virtual void mf3(void) &;
	void mf4(void) const;
};
 
class Derived : public Base {
public:
	virtual void mf1(void);           // 기반 함수와 const성이 다름
	virtual void mf2(unsigned int x); // 기반 함수와 매개변수가 다름
	virtual void mf3(void) &&;        // 기반 함수와 참조 한정사가 다름
	void mf4(void) const;             // 기반 함수에 virtual 키워드가 없음
};
```

문제없이 overriding 이 성공하는 경우는 다음과 같음
```cpp
class Base {
public:
	virtual void mf1(void) const;
	virtual void mf2(int x);
	virtual void mf3(void) &;
	virtual void mf4(void) const;
};
 
class Derived : public Base {
public:
	virtual void mf1(void) const override;
	virtual void mf2(int x) override;
	virtual void mf3(void) & override;
	void mf4(void) const override; // virtual 생략 가능
};
```

이처럼 overriding 요구 조건이 까다롭기에 실수가 발생할 가능성이 높지만 코드는 정상적으로 실행되는 경우가 많음. 따라서 overriding 실패할 경우 경고해줄 방법이 필요함.


### override, final 키워드

C++ 11 에서는 `override` 와 `final` 키워드를 추가하여 컴파일러가 overriding 을 알려줄 수 있음.
 `override` 는 기반 클래스에 있는 함수를 overriding 한다는 의미
`final`은 기반 클래스에 있는 함수를 마지막으로 overriding 한다는 의미


## 멤버 함수 참조 한정사

앞에서 참조 한정사를 통해 멤버 함수가 호출되는 객체 (*this)의 lvalue 버전과 rvalue 버전을 다른 방식으로 처리할 수 있음.

```cpp
class Widget {
public:
	using DataType = std::vector<double>;
	DataType& data(void) { return values; }
private:
	DataType values;
};

Widget w; 				// Widget 타입의 w 변수
Widget makeWidget(); 	// Widget 타입의 객체를 반환하는 팩토리 함수

auto vals1 = w.data();             // w.values를 vals1에 복사
auto vals2 = makeWidget().data();  // 임시 객체 Widget의 values를 vals2에 복사
```

`data()` 멤버 함수를 호출하는 객체가 lvalue든 rvalue 든 상관없이 리턴 타입은 lvalue 이고, 이를 사용해 vals1, vals2 객체를 생성할 때 복사 생성자가 호출됨.
따라서, 임시 객체(rvaule) 안의 멤버 변수 값을 가져올 때도 비효율적인 복사 연산을 수행함.
이때 참조 한정사를 사용하면 효율적으로 구현 가능함.

```cpp
class Widget {
public:
	using DataType = std::vector<double>;
	DataType& data(void) & { return values; } //lvalue 객체에 대해서는 lvalue 반환              
    DataType&& data(void) && { return std::move(values); } // rvalue 객체에 대해서는 rvalue 반환
private:
	DataType values;
};

Widget w; 				// Widget 타입의 w 변수
Widget makeWidget(); 	// Widget 타입의 객체를 반환하는 팩토리 함수

auto vals1 = w.data();            // lvalue 참조 한정사 함수를 호출, vals1은 복사 생성됨
auto vals2 = makeWidget().data(); // rvalue 참조 한정사 함수를 호출, vals2은 이동 생성됨
```
멤버 함수를 호출하는 객체가 lvalue 이면 lvalue 를 반환하고, rvalue 이면 rvalue 를 반환하는 data 함수를 구현하여 vals1 객체 생성시 복사 생성자가 vals2 객체 생성시 이동 생성자가 호출되도록 할 수 있음.



## Item 13. iterator 보다 const_iterator 를 선호

`const_iterator`는 const 를 가리키는 포인터의 STL 버전
`const_iterator`를 사용하면 `const_iterator`가 가리키는 원소의 값을 변경할 수 없음.

### C++98 에서 const_iterator 를 사용하기 힘듦

C++ 98 dptjsms `const_iterator`를 생성하고 활용에 제약이 있었음.

```cpp
std::vector<int> values;

// values의 원소값이 1983인 원소를 찾아서 가리키는 iterator 생성
// (없으면 value.end()를 가리키고 있음) 
std::vector<int>::iterator it = std::find(values.begin(), values.end(), 1983);

// iterator가 가리키는 위치에 새로운 원소 삽입, 만약 찾은 원소가 없었으면 맨 뒤에 삽입
values.insert(it, 1998);
```
위와 같은 상황에서는 `iterator`가 가리키는 원소를 수정하지 않으므로 `const_iterator`를 사용하는 것이 바람직함.

```cpp
typedef std::vector<int>::iterator IterT;
typedef std::vector<int>::const_iterator ConstIterT;

std::vector<int> values;

ConstIter ci = std::find(static_cast<ConstIterT>(values.begin(), // 캐스팅
                         static_cast<ConstIterT>(value.end()), 
                         1983);

values.find(static_cast<IterT>(ci), 1998) // 컴파일이 안될 수 있음
```
하지만 C++ 98에서  `const_iterator`를 사용하면 제약들이 존재함.
C++ 98 에서는 begin(), end() 함수를 호출한 컨테이너가 const 가 아닐 경우 `const_iterator`를 얻을 수 없음. 따라서 `iterator`를 `const_iterator`로 형변환해서 사용해야 함.

또한, C++ 98 에서 컨테이너는 삽입, 삭제 위치를 `iterator`로만 지정할 수 있었음. 따라서 `const_iterator`를 `iterator`로 형변환해서 사용해야 함. 하지만 C++ 98 / C++ 11 는 `const_iterator`에서 `iterator`로 이식성 있는 변환이 존재하지 않음.


### C++ 11 부터 const_iterator 를 사용하는 방법

C++ 11 에서는 컨테이너 멤버 함수인 cbegin(), cend() 는 `const_iterator`를 반환해주고, 컨테이너의 삽입, 삭제 위치를 `const_iterator`로 지정할 수 있게 됨.

```cpp
std::vector<int> values;

auto it = std::find(values.cbegin(), values.cend(), 1983);
values.insert(it, 1998);
```

#### const_iterator 주의사항

템플릿을 통해 보편적인(general) 상황에서 모두 사용할 수 있는 코드를 구현 시 주의해야 함.
바로 `iterator`를 반환하는 함수 (begin, end, cbegin, cend 등) 를 멤버 함수가 아닌 비멤버 함수로 사용하는 것이 바람직함. 그 이유는 모든 객체가 `iterator` 를 반환하는 멤버 함수를 가지고 있다는 보장이 없기 때문임.

```cpp
template <typename C, typename V>
void findAndInsert(C& container, const V& targetVal, const V& insertVal) {
	using std::cbegin;
	using std::cend;

	// 비 멤버 함수인 cbegin, cend를 사용
	auto it = std::find(cbegin(container), cend(container), targetVal);
	container.insert(it, insertVal);
} 
```
이 템플릿은 C++ 14에서는 잘 작동하지만 C++ 11 에서는 작동하지 않음.
C++ 11 에서는 비멤버 함수 begin 과 end 는 추가했지만, cbegin, cend, rbegin, rend, crbegin, crend 는 추가하지 않음.
따라서 C++ 11 에서 템플릿 코드가 작동하기 위해서는 다음과 같이 누락된 비멤버 함수를 구현해야 함.

```cpp
template <class C>
auto cbegin(const C& container) -> decltype(std::begin(container)) {
	return std::begin(container);
}
```
컨테이너 타입 C 를 const C& 로 참조할 경우 begin 함수는 const_iterator 타입을 반환함. 이를 통해 비멤버 함수로 begin 함수만 제공해도 cbegin 함수를 사용할 수 있음.



## Item 14. 예외를 방출하지 않을 함수는 noexcept로 선언

C++ 98에서 예외 명세의 비효율적임. 프로그래머는 함수를 방출할 수 있는 예외 타입을 명세해야 했으며 함수의 구현을 수정하면 예외 명세도 바꾸어야 할 가능성이 있었음. 즉, 함수 구현과 예외 명세, 클라이언트 코드 사이의 일관성 유지에 아무런 도움을 주지 않았음.

C++ 11 부터는 예외를 하나라도 방출할 수 있는 함수인지 여부만이 의미 있는 정보라고 판단하여 C++ 98 스타일의 예외 명세는 비권장(deprecate) 기능으로 분류됨.

### noexcept

C++ 11 부터 `noexcept` 키워드를 통해 예외를 방출할 수 있는 함수인지 아닌지 이분법적으로 나눌 수 있게 됨. 함수 선언 뒤에 `noexcept` 키워드를 붙이면 해당 함수는 예외를 방출하지 않음을 뜻함.

#### noexcept 의 장점

1. 함수의 예외 방출 여부를 명확하게 알 수 있음. 
인터페이스를 설계할 때 함수에 `noexcept` 를 사용할 것인지 결정해야 함. 사용자는 함수의 인터페이스를 통해 예외 방출 여부를 명확하게 알 수 있어 예외 안정성이나 효율성을 고려하여 호출 코드를 구현 가능.

2. 컴퍼일러 최적화
```cpp
int f() throw();  // f 함수는 예외를 방출하지 않음, C++98 방식
int f() noexcept; // f 함수는 예외를 방출하지 않음, C++11 방식
```
해당 함수가 예외를 방출하지 않는다는 선언을 위의 코드처럼 할 수 있고, 이때 예외를 방출하지 않는 다고 선언한 함수 내부에 예외를 방출하는 코드가 존재할 경우 다음과 같이 작동한다.

C++ 98 에서는 예외가 발생한다면 함수가 호출된 지점까지 호출 스택이 풀리며, 그 지점에서 몇 가지 동작이 취해진 후 종료됨.

C++ 11 에서는 예외가 발생한다면 호출 스택이 풀릴 수도 있고 아닐 수도 있음. 즉, 최적화 유연성이 있음. `noexcept`함수에서 예외가 발생한다고 해도 함수 호출 시점까지 스택을 풀기 가능 상태로 유지할 필요도 없고, 함수 안의 객체들이 반드시 생성의 반대 순서도 파괴할 필요도 없음.

```cpp
ReturnType function(params) noexcept; // 최적화 여지가 가장 큼
ReturnType function(params) throw();  // 최적화 여지가 더 작음
ReturnType function(params);          // 최적화 여지가 더 작음
```

3. STL 최적화에 유용함.

예시로 std::vector 의 push_back 함수가 있음.

```cpp
std::vector<Widget> vw;
Widget w;
vw.push_back(w);
```
std::vector 는 내부에 일정한 크기의 메모리를 할당해서 사용하고 있지만, 원소가 많아져 메모리가 가득 찬 상태에서 삽입을 하면 내부에서 재할당이 일어남. 
이 과정에서 복사가 일어날 경우 예외가 발생하더라도 예외 안정성을 보장할 수 있음. 즉, 요소들을 복사하는 도중에 예외가 발생해도 기존 메모리에 있는 데이터는 그대로 이기 때문.

하지만 이 과정에서 이동이 일어날 경우 예외가 발생하면  예외 안정성을 보장할 수 없음. 이동 도중 예외가 발생하면 기존의 메모리에 있는 데이터가 달라졌을 수 있기 때문. 
따라서 C++ 11 컴파일러는 이동 연산이 예외를 방출하지 않음이 확실한 경우에만 이동 연산을 수행함. 이때 `noexcept` 키워드가 컴파일러에서 예외를 방출하지 않음을 보장해줌.

또 다른 예시로 swap 함수가 있음. swap 은 여러 STL 알고리즘 구현에서 핵심 구성요소이며, 복사 대입 연산자에서도 흔히 쓰임. 따라서 `noexcept` 를 통해 최적화할 가치가 큼.

```cpp
template <class T, size_t N>
void swap(T (&a)[N], T (&b)[N]) noexcept(noexcept(swap(*a, *b)));

template <class T1, class T2>
struct pair {
	void swap(pair& p) noexcept(noexcept(noexcept(swap(first, p.first)) &&
		                                 noexcept(swap(second, p.second)));
}
```
위의 함수들은 `조건부 noexcept` 이다. 즉, 이들이 `noexcept`인지의 여부는 `noexcept` 안의 표현식들이 `noexcept`인지에 의존함. 간단하게 말해서 `noexcept(expr)` 안의 `expr`이 참(`noexcept`)이어야 함수도 `noexcept`로 선언한다는 뜻임.


### noexcpet 결정

`noexcept`는 함수의 인터페이스의 일부이므로, 예외를 방출하지 않는다는 성질을 오랫동안 유지할 결심이 선 경우에만 선언해야 한다. 
대부분의 함수가 예외에 중립적이다. 이는 스스로 예외를 던지지는 않지만, 예외를 던지는 다름 함수들을 호출할 수 있다. 다른 함수가 예외를 던지면 예외 중립적 함수는 그 예외를 그대로 통과시킨다. 따라서 예외 중립적 함수는 `noexcept`가 될 수 없다.

기본적으로 모든 메모리 해제 함수와 모든 소멸자는 암묵적으로 `noexcept`이다. 따라서 직접 `noexcept`로 선언할 필요가 없음. 만약 이러한 함수에서 예외를 방출하고 싶다면 `noexcept(false)`를 사용하면 되지만 하지 않는 것이 좋다.



## Item 15. 가능하면 항상 constexpr 을 사용

### 상수 표현식 (constant expression)
컴파일 시점에 어떠한 식의 값을 결정할 수 있다면 해당 식을 상수 표현식이라고 한다. 또한, 이러한 상수 표현식 중 값이 정수인 것들을 정수 상수 표현식이라고 부르고, 이는 배열 크기나 정수 템플릿 인수, 열거자 값 등을 지정할 때  사용할 수 있다. 

### constexpr

C++ 11 부터 도입된 `constexpr` 키워드는 해당 식이 상수 표현식임을 명시해주는 키워드임. `constexpr`로 객체를 선언하면 해당 객체는 컴파일 시점에 상수 표현식으로 정의된다 ( 선언과 동시에 초기화가 되어야 함)

```cpp
int sz;								// 비 constexpr 변수

constexpr auto arraySize1 = sz;    	// 에러, sz의 값이 컴파일 시점에 알려지지 않음
std::array<int, sz> data1;         	// 에러, sz의 값이 컴파일 시점에 알려지지 않음

const auto arraySize2 = sz;        	// 가능, arraySize2는 sz의 const 복사본
std::array<int, arraySize2> data2; 	// 에러, arraySize2의 값이 컴파일 시점에 알려지지 않음

constexpr auto arraySize3 = 10;    	// 가능
std::array<int, arraySize3> data2; 	// 가능, arraySize3는 constexpr 객체

```

`const`가 `constexpr`에서와 동일한 보장을 제공해주지 않음. `const`의 경우 객체가 반드시 컴파일 시점에서 알려진 값으로 초기화 되지는 않고, `constexpr`는 반드시 컴파일 시점에 값이 정해져 있어야 한다.
간단히 말해서, 모든 `constexpr`객체는 `const` 이지만 모든 `const` 객체가 `constexpr` 인 것은 아님.

### constexpr 함수
`constexpr` 함수는 컴파일 시점 상수를 인수로 해서 호출된 경우에는 컴파일 시점 상수를 산출한다. 만약 인자의 값이 컴파일 시점에 알려지지 않는다면 `constexpr` 함수는 컴파일 시점에 실행할 수 없고 일반 함수처럼 동작한다 ( 이 경우 constness 보장할 수 없음).

예를 들어, 컴파일 도중에 $3^n$의 크기를 갖는 배열을 구현한다고 가정해보자 ( $n$은 컴파일 도중 알려지고, 배열의 값은 int 이다).

이때, C++ 표준 라이브러리 함수로는 std::pow 가 있지만 다음과 같은 문제로 사용할 수 없고, 직접 작성해야 한다.
1. std::pow 함수는 리턴 타입이 int 가 아닌 double 이다.
2. std::pow 함수는 컴파일 시점에 값이 결정되는 `constexpr`이 아니다.

```cpp
constexpr 
int pow(int base, int exp) noexcept { 	// 예외를 던지지 않는 constexpr 함수
	...									// 구현은 나중에
}

constexpr auto numConds = 5;                // 지수의 값
std::array<int, pow(3, numConds)> results; 	// 3^n의 크기를 갖는 배열

// 만약, pow 함수의 인자가 컴파일 시점에 알려지지 않으면 일반 함수처럼 호출
auto base = readFromDB("base");			// 실행시점에서 값들을 구함
auto exp = readFromDB("exponent");

auto baseToExp = pow(base, exp);		// 실행시점에서 pow 함수 호출
```

`constexpr` 함수를 구현할 때는 제약이 존재하는데, C++ 11과 C++ 14의 차이가 존재한다.
C++ 11 에서 `constexpr`함수는 실행 가능 문장이 많아야 하나이어야 하고, C++ 14 에서는 제약이 느슨해져, 다음과 같은 구현이 허용됨.

```cpp
// C++11에서는 constexpr 함수는 실행 가능한 문장이 최대 1개
// 따라서 삼항 연산자나 재귀 등 여러 방법으로 줄여야 함.
constexpr int pow(int base, int exp) noexcept {
	return (exp == 0 ? 1 : base * pow(base, exp - 1));
}

// C++14에서는 constexpr 함수는 실행 가능한 문장의 제약이 없음
constexpr int pow(int base, int exp) noexcept {
	auto result = 1;
	for (int i = 0; i < exp; ++i) result *= base;
	return result;
}
```

또한, `constexpr` 함수는 반드시 리터럴 타입 (literal type) 들을 받고 돌려주어야 한다.

	 리터럴 타입(literal type)은 컴파일 도중에 값을 결정할 수 있는 타입
	 
C++ 11 에서는 void 를 제외한 모든 내장 타입은 리터럴 타입에 해당한다. 그리고 생성자와 적절한 멤버 함수들이 `constexpr` 인 사용자 타입도 리터럴 타입이 될 수 있다.

```cpp
class Point {
public:
	// 컴파일 시점에 값을 알 수 있다면 constexpr 생성자도 가능
	constexpr Point(double xVal = 0, double yVal = 0) noexcept 
	: x(xVal), y(yVal) 
	{}
	constexpr double xValue(void) const noexcept { return x; }
	constexpr double yValue(void) const noexcept { return y; }

	// C++11에서 void는 리터럴 타입이 아님
	void setX(double newX) noexcept { x = newX; }
	void setY(double newY) noexcept { y = newY; }
	
private:
	double x, y;  
};

constexpr Point p1(9.4, 27.7); 	// ok, constexpr 생성자가 컴파일 타입에 실행
constexpr Point p2(28.8, 5.3);	// ok

constexpr 
Point midpoint(const Point& p1, const Point& p2) noexcept 
{	// constexpr 멤버 함수들을 호출
	return { (p1.xValue() + p2.xValue()) / 2, (p1.yValue() + p2.yValue()) / 2 };
}

// constexpr 함수의 결과를 이용해 constexpr 객체 초기화
constexpr auto mid = midpoint(p1, p2);
```

Point 의 생성자를 `constexpr` 로 선언할 수 있는 이유는, 주어진 인수들이 컴파일 시점에서 알려진다면 생성된 Point 객체의 멤버 변수들의 값 역시 컴파일 시점에 알려질 수 있기 때문이다. 따라서 Point 객체는 `constexpr` 객체가 될 수 있다. 마찬가지로 `constexpr` Point 객체의 조회용 멤버 함수 xValue, yValue 역시 `constexpr`이 될 수 있다. 결과적으로 Point 조회 함수들을 호출한 결과들로 또 다른 `constexpr` 객체를 초기화 하는 `constexpr` 함수를 작성하는 것도 가능하다.

하지만 C++ 11 에서는 setX, setY 함수는 두 가지 제약 때문에 `constexpr`로 선언할 수 없다.

1. 해당 함수는 멤버 변수를 수정하는 setter 함수인데, C++ 11 에서는 `constexpr` 객체가 암묵적으로 `const`로 선언됨.
2. 해당 함수는 리턴 타임이 void 인데, C++ 11 에서는 void 는 리터럴 타입이 아님.

C++ 14 에서는 두 가지 제약 모두 사라졌다.
```cpp
class Point {
public:
	...
	// C++14부터는 가능
	constexpr void setX(double newX) noexcept { x = newX; }
	constexpr void setY(double newY) noexcept { y = newY; }
	...
};

// 원점을 기준으로 p와 대칭인 Point 객체를 반환
constexpr Point reflection(const Point& p) noexcept {
	Point result;				// 비const Point 생성
	
	result.setX(-p.xValue());
	result.setY(-p.yValue());
	
	return result;				// 그 복사본을 반환
}

// 컴파일 시점에 전부 가능
constexpr Point p1(9.4, 27.7);
constexpr Point p2(28.8, 5.3);
constexpr auto mid = midpoint(p1, p2);
constexpr auto reflectedMid = reflection(mid); // (-19.1, -16.5) 이는 컴파일 도중 알려짐
```

`constexpr` 키워드는 객체나 함수의 인터페이스의 일부이다. 따라서 설계 단계에서 잘 결정해야 한다. 중간에 `constexpr` 를 빼버리면 큰 문제가 생긴다.



## Item16. const 멤버 함수를 스레드에 안전하게 작성

수학에서 다항식(polynomial) 을 클래스로 구현한다고 생각해보자. 이 클래스가 유용하려면 다항식의 근(root) 들을 구하는 멤버 함수들을 구현해야 한다. 이 함수는 다항식을 수정하지 않을 것이므로, `const`로 선언하는 것이 자연스럽다.

```cpp
class Polynomial {
public:
	using RootsType = std::vector<double>;
	RootsType roots() const;
};
```
또한, 다항식 근의 계산 비용이 클 수 있으므로, 꼭 필요할 때만 계산하는 것이 바람직하고, 한 번 계산하면 그 값을 캐싱하여 여러 번 계산할 필요가 없게 구현하는 것이 바람직하다.

```cpp
class Polynomial {
public:
	using RootsType = std::vector<double>;
	RootsType roots() const {
	    if (!rootsAreValid) {	// 캐싱 여부 확인
		    // 근을 계산하고 결과값을 rootVals에 저장
		    rootsAreValid = true;
		}
		return rootsVals;
	}
private:
	mutable bool rootsAreValid{ false };
	mutable RootsType rootVals{};
};
```

이때, roots 멤버 함수는 다항식의 내용을 변경하지 않지만 캐싱을 위해 rootVals 와 rootsAreValid 멤버 변수를 변경할 수 있어야 한다. 따라서 const 로 선언한 멤버 함수 안에서 멤버 변수들의 값을 변경하기 위해서는 멤버 변수를 `mutable` 로 선언해야 한다.

### 멀티스레드 환경에서의 구현

```cpp
Polynomial p;

/*------- 스레드 1-------*/       /*------- 스레드 2-------*/ 
auto RootsOfP = p.roots();        auto valsGivingZero = p.roots();
```

구현한 roots 함수는 문제없이 동작하지만 멀티스레드 환경에서 사용한다면 문제가 발생한다. 위 처럼 두 스레드가 동시에 roots 함수에 접근하여 멤버 변수를 변경하다보면 예상치 못한 문제가 발생할 수 있기 때문이다. 이러한 문제를 해결하기 위해 가장 쉬운 방법은 동시화 수단, 즉 std::mutetx 를 사용하는 것이다.

#### std::mutex

뮤텍스는 여러 스레드들이 동시에 같은 코드에 접근하는 것을 막아준다. 
std::mutex 를 사용하여 roots 함수에서 발생하는 동시성 문제를 해결하고 스레드 안정성이 있는 코드를 작성할 수 있다. 

```cpp
class Polynomial {
public:
	using RootsType = std::vector<double>;

	RootsType roots() const {
	    std::lock_guard<std::mutex> g(m); // 뮤텍스 lock
    
	    // 캐싱 여부 확인
	    if (!rootsAreValid) {
		    // 근을 계산하고 결과값을 rootVals에 캐싱함
		    rootsAreValid = true;
	    }

	    return rootsVals;
	} // 뮤텍스 unlock
private:
	mutable std::mutex m;
	mutable bool rootsAreValid{false};
	mutable RootsType rootVals{};
};
```

하지만 std::mutex 를 멤버 변수로 가지고 있으면 해당 객체의 복사, 이동이 불가능하다.

만약 std::mutex을 사용할 때보다 비용을 줄이고 싶다면, std::atomic 을 사용할 수도 있다.

#### std::atomic

std::atomic 은 연산을 원자적으로 할 수 있도록 지원하는 템플릿 클래스이다.
멀티스레드 환경에서 멤버 함수의 호출 횟수를 세고 싶다면 다음과 같이 구현할 수 있다.

```cpp
class Point {
public:
	double distanceFromOrigin() const noexcept {
	    ++callCount;	// 원자적 증가
    
	    return std::hypot(x, y); // C++11에서 지원하는 2차원 거리 계산 함수
	}
private:
	mutable std::atomic<unsigned> callCount{ 0 };
	double x, y;
};
```

다만 std::atomic 또한 멤버 변수로 가지고 있으면 해당 객체의 복사, 이동이 불가능하다.

####  std::mutex 와  std::atomic 는 적절하게 선택하여 사용 필요

 std::atomic은 std::mutex 보다 비용이 더 싸다는 장점이 있지만, 남용해서는 안된다. 
 예를 들어 계산 비용이 큰 값을 캐시에 저장하는 클래스에 std::atomic 을 사용해보자.

```cpp
class Widget {
public:
	int magicValue() const {
		if (cacheValid) return cachedValue;
		else {
			auto val1 = expensiveComputation1();
			auto val2 = expensiveComputation2();
			cachedValue = val1 + val2;   // 1번 코드
			cacheValid = true;           // 2번 코드
			return cachedValue;
		}
	}
private:
	mutable std::atomic<bool> cacheValid{false};
	mutable std::atomic<int> cachedValue;
};
```

첫번째 스레드가 magicValue 를 호출하여 cachedValue 값을 계산하고 cacheValid 를 true 로 변경하기 전에 두번째 스레드가 magicValue 호출하여 cachedValue 값을 또 계산하는 문제가 발생할 수 있다. 이문제를 해결하기 위해 1번 코드와 2번코드의 순서를 바꾸면 더 큰 문제가 발생한다. 
```cpp
class Widget {
public:
	int magicValue() const {
		if (cacheValid) return cachedValue;
		else {
			auto val1 = expensiveComputation1();
			auto val2 = expensiveComputation2();
			cacheValid = true;           // 2번 코드
			cachedValue = val1 + val2;   // 1번 코드
			return cachedValue;
		}
	}
private:
	mutable std::atomic<bool> cacheValid{false};
	mutable std::atomic<int> cachedValue;
};
```

첫번째 스레드가 cacheValid 를 true 로 변경하고 cachedValue 값을 계산하기 전에 두번째 스레드에서 계산 값이 들어가있지 않은 cachedValue 를 리턴하는 문제가 발생할 수 있다.

이러한 경우는 std::mutex 를 사용한다면 문제를 해결할 수 있다.

```cpp
class Widget {
public:
	int magicValue() const {
		std::lock_guard<std::mutex> guard(m); // 뮤텍스 lock
		if (cacheValid) return cachedValue;
		else {
			auto val1 = expensiveComputation1();
			auto val2 = expensiveComputation2();
			cachedValue = val1 + val2;
			cacheValid = true;
			return cachedValue;
		}
	}
private:
	mutable std::mutex m;
	mutable int cachedValue;
	mutable bool cacheValid{ false };
};
```

따라서, 동기화가 필요한 변수 또는 주소가 하나일 경우 std::atomic 을 사용하는 것이 적합하지만, 둘 이상의 변수나 주소가 동기화가 필요할 경우 std::mutex 를 사용하는 것이 바람직하다. 



## Item17. 특수 멤버 함수들의 자동 작성 조건을 숙지


### 특수 멤버 함수 (special member function)

C++ 이 스스로 작성해주는 멤버 함수들을 특수 멤버 함수라고 부른다. C++ 98 에서는 기본 생성자, 소멸자, 복사 생성자, 복사 대입 연산자가 만들어지고 C++ 11에서는 이동 생성자, 이동 대입 연산자가 추가됨. 

### C++ 에서 특수 멤버 함수들의 자동 작성 조건

#### 기본 생성자 ( default constructor )
클래스에 사용자가 직접 선언한 생성자가 없을 경우에만 자동으로 작성된다.

#### 소멸자  ( destructor )
클래스에 사용자가 직접 선언한 소멸자가 없을 경우에만 자동을 작성된다.
C++ 11 에서는 소멸자가 기본적으로 `noexcept` 이다.
기본적으로 작성되는  소멸자는 오직 기반 클래스 소멸자가 `virtual` 일때만 `virtual`이다.

#### 복사 생성자 ( copy constructor )와 복사 대입 연산자(copy assignment operator)
자동으로 작성되는 복사 생성자와 복사 대입 연산자는 static 이 아닌 멤버 변수를 복사한다. 
클래스에 사용자가 직접 선언하지 않은 경우에만 복사 생성자와 복사 대입 연산자가 자동으로 생성된다. 하지만 이동 연산이 하나라도 선언되어 있다면 삭제(비활성화)된다.

두 복사 연산은 서로 독립적이다. 즉, 하나를 선언한다고 해서 다른 하나의 작성이 방지 되지 않는다. 
복사 생성자를 선언하고 복사 대입 연산자를 선언하지 않은 경우 컴파일러는 자동으로 복사 대입 연산자를 생성해준다. 반대도 마찬가지다.

그리고 3의 법칙(Rule of Three) 이라는 지침이 있다. 이는 복사 생성자, 복사 대입 연산자, 소멸자 중 하나라도 선언했으면 나머지도 다 선언해야 한다는 것이다. 하나라도 선언하였다면 이는 메모리 관리도 필요한 경우이기 때문이다. 하지만 이를 강제하지는 않는다. 강제한다면 기존의 코드에 문제가 생길 가능성이 있기 때문이다. 

#### 이동 생성자(move constructor)와 이동 대입 연산자(move assignment operator)
자동으로 작성되는 이동 생성자와 이동 대입 연산자는 static 이 아닌 멤버 변수를 모두 이동한다. 만약 이동할 수 없는 타입들은 복사한다. 
이동 생성자와 이동 대입 연산자는 자동 생성 조건이 까다롭다.
클래스에 사용자가 직접 선언한 복사 연산, 이동 연산, 소멸자가 없을 때만 이동 생성자와 이동 대입 연산자가 자동으로 생성된다. 하나라도 있으면 자동으로 생성되지 않는다. 

두 이동 연산은 서로 독립적이지 않다. 즉, 둘 중 하나라도 선언하면 컴파일러는 다른 하나를 선언하지 않는다. 
예를 들어, 클래스의 이동 생성자를 선언했다면, 컴파일러가 자동으로 작성해주는 기본적인 이동 생성이 그 클래스에 적합하지 않아서 다른 방식으로 구현해야 하기 때문일 것이다. 그리고 이동 생성이 적합하지 않다면 이동 대입도 적합하지 않을 가능성이 크다. 따라서 하나가 선언되어 있다면 다른 하나를 자동으로 작성해주지 않는다. 



### default
C++ 11 에서는 `default` 키워드를 통해 컴파일러가 자동으로 작성해주는 특수 멤버 함수를 사용하겠다고 명시적으로 표현할 수 있다.

```cpp
class Widget{
public:
	Widget() = default;
	~Widget() = default;

	Widget(const Widget& w) = default; 				// 기본 복사 생성자
	Widget& operator=(const Widget& w) = default; 	// 기본 복사 대입 연산자

	Widget(Widget&& w) = default;					// 기본 이동 생성자
	Widget& operator=(Widget&& w) = default;		// 기본 이동 대입 연산자
};
```

`default`를 사용하면 특수 멤버 함수가 자동으로 작성이 안되는 상황에서도  특수 멤버 함수를 사용할 수 있다. 

또한 특수 멤버 함수를 사용하겠다는 의도를 명확하게 표현할 수 있으며, 상당히 미묘한 버그들을 피하는 데에도 도움이 된다. 

문자열을 찾는 자료구조를 가지고 있는 클래스가 있다고 해보자.
```cpp
class StringTable{
public:
	StringTable() {}
private:
	std::map<int, std::string> values;
};
```
이 클래스가 복사 연산들과 이동 연산들, 그리고 소멸자를 전혀 선언하지 않는다면, 컴파일러는 해당 함수들을 자동으로 작성해준다. 그러다 이런 객체들의 기본 생성과 소멸을 기록하는 것이 유용하겠다고 생각하여 다음과 같이 추가한다고 하자.
```cpp
class StringTable{
public:
	StringTable() { makeLogEntry("Creating StringTable object"); }
	~StringTable() { makeLogEntry("Destroting StringTable object"); }
	...	// 다른 함수들은 이전과 동일

private:
	std::map<int, std::string> values;
};
```

이는 합리적인 해결책으로 보이지만, 소멸자를 작성해버리면 특수 멤버 함수 자동 작성 규칙에 따라 복사 생성자, 복사 대입 연산자만 자동 생성되고 , 이동 연산들은 자동으로 생성되지 않는다. 따라서 이동이 필요한 상황에서도 복사를 하여 성능이 떨어지는 문제가 발생할 수 있다. 

이때 `default` 를 사용하여 명시적으로 이동 연산을 정의하여 문제를 해결할 수 있다. 
```cpp
class StringTable{
public:
	StringTable() { makeLogEntry("Creating StringTable object"); }
	~StringTable() { makeLogEntry("Destroting StringTable object"); }


	StringTable(StringTable&& st) = default;
	StringTable& operator=(StringTable&& st) = default;
	...	// 다른 함수들은 이전과 동일

private:
	std::map<int, std::string> values;
};
```

#### 템플릿 멤버 함수 사용시 자동 작성 규칙

템플릿 멤버 함수를 사용한다고 특수 멤버 함수의 자동 작성 조건이 비활성화 되지 않는다.

```cpp
class Widget {
public:
	template <typename T>
	Widget(const T& rhs);

	template <typename T>
	Widget& operator=(const T& rhs);
};
```

만약 T 가 Widget 일 경우 해당 템플릿 멤버 함수는 복사 생성자, 복사 대입 연산자와 같은 모양의 함수를 인스턴스화하지만, 그래도 컴파일러는 복사 생성자, 복사 대입 연산자, 이동 생성자, 이동 대입 연산자를 자동으로 작성한다(규칙이 만족되었다고 할 때).