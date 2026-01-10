# Flanki Shooter

Projekt zaliczeniowy z przedmiotu **Podstawy informatyki**.

Gra typu **2D shooter**, w której gracz steruje **studentem** poruszającym się
w poziomie na dole ekranu. Celem gry jest strzelanie do **spadających puszek piwa**,
zdobywanie punktów oraz unikanie utraty **HP**.

Power-up **kawa** pomaga studentowi przetrwać
oraz chwilowo zwiększa szybkostrzelność.


## Cel projektu
Celem projektu było:
- stworzenie działającej gry 2D w języku **C++**,
- wykorzystanie biblioteki **raylib**,
- spełnienie wymagań projektowych,
- praca zespołowa z wykorzystaniem systemu kontroli wersji **Git**.

Projekt rozwijany był **krok po kroku**, a każda funkcjonalność została dodana
w osobnym commicie.


## Wymagania projektowe (zaliczenie)

W projekcie zostały wykorzystane:

- **funkcje**
- **pętle** (`for`, `while`)
- **instrukcje warunkowe** (`if`)
- **tablice** (m.in. `array`, tablice obiektów gry)
- **operacje na plikach** (zapis i odczyt profilu gracza do `profile.dat`)
- **szyfrowanie danych** (prosty XOR)
- **elementy programowania strukturalnego / OOP** (struktury obiektów gry)
- **interfejs użytkownika**:
  - menu główne
  - logowanie nickiem
  - wyświetlanie statystyk profilu
- **dodatkowe mechaniki rozgrywki**:
  - punkty życia (HP)
  - poziom upicia
  - power-up (kawa)
  - rosnący poziom trudności


## Technologie
- **Język:** C++
- **Biblioteka graficzna:** raylib
- **System budowania:** CMake
- **System kontroli wersji:** Git / GitHub


## Sterowanie
- **A / D** lub **← / →** – ruch studenta
- **Spacja** lub **LPM** – strzał
- **ESC** – powrót do menu / wyjście


## Plik profilu
Po zakończeniu gry zapisywany jest profil gracza:

- plik: `profile.dat`
- zapisywane dane:
  - nick gracza
- dane są zabezpieczone prostym szyfrowaniem


## Uruchomienie – Visual Studio 2022 (Windows)

1. **File → Open → Folder…**
2. Wybierz folder z `CMakeLists.txt`
3. **Build → Build All**
4. **Debug → Start Without Debugging** (`Ctrl + F5`)

Przy pierwszym uruchomieniu Visual Studio pobierze bibliotekę raylib
(wymagane połączenie z internetem).


