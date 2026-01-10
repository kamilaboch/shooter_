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
  - najlepszy wynik
  - liczba trafień
  - liczba rozegranych gier
- dane są zabezpieczone prostym szyfrowaniem (XOR)


## Uruchomienie – Visual Studio 2022 (Windows)

1. **File → Open → Folder…**
2. Wybierz folder z `CMakeLists.txt`
3. **Build → Build All**
4. **Debug → Start Without Debugging** (`Ctrl + F5`)

Przy pierwszym uruchomieniu Visual Studio pobierze bibliotekę raylib
(wymagane połączenie z internetem).

// ZMIANY



Jasne, oto czysta, profesjonalna wersja pliku README.md bez żadnych emotek, oparta wyłącznie na punktorach i tekście.

Skopiuj poniższą treść i wklej do pliku README.md na GitHubie.

Flanki Simulator: Student Edition
Symulator przetrwania sesji, gdzie twoim wrogiem nie są egzaminy, ale procenty.

Wciel się w rolę studenta, który musi stawić czoła nadciągającej armii puszek. Twoim celem jest przetrwać 10 poziomów, nie zaliczając "Zgonu" (100% upojenia). Czy dasz radę pokonać Kuflowe Mocne i uniknąć mandatów?

Sterowanie
W, S, A, D / Strzałki - Poruszanie się (uwaga na bezwładność po alkoholu!)

SPACJA - Strzelanie piłeczkami

SPACJA (szybkie wciskanie) - Zerowanie piwa (w Bonus Levelu)

ENTER - Zatwierdź / Start Gry

N - Nowy Profil (w Menu Głównym)

Mechaniki Gry
Fizyka Upojenia
To nie jest zwykła strzelanka. Im więcej "obrażeń" otrzymasz (czyli im więcej wypijesz), tym trudniej sterować postacią.

0% Upojenia: Pełna kontrola, ostre hamowanie.

50% Upojenia: Postać zaczyna się ślizgać (efekt driftu).

100% Upojenia: ZGON (Koniec Gry).

Ściana Piwa
Wrogowie to szerokie na 300 pikseli "mury", które nacierają na ciebie falami. Musisz przebić się przez ich obronę, zanim dotrą do dołu ekranu.

Boss: Kuflowe Mocne (Level 10)
Ostateczne wyzwanie. Gigantyczny boss, który posiada unikalne ataki:

Kebab: Spada prosto w dół. Trafienie dodaje +15% upojenia.

Mandat: Pocisk samonaprowadzający. Trafienie dodaje +25% upojenia (stres).

Bonus Level: Zerowanie
Co 3 poziomy (3, 6, 9) następuje wyzwanie "Chug Challenge". Musisz jak najszybciej klikać SPACJĘ, żeby wyzerować kufel przed upływem czasu. Nagroda to regeneracja i punkty.

Power-upy (Znajdźki)
Wypadają losowo z pokonanych puszek. Symbolizują je znaki tekstowe:

[ + ] Kroplówka: Trzeźwienie (zmniejsza % upojenia).

[ % ] Dopitka: Dodatkowe punkty, ale lekko zwiększa upojenie.

[ M ] Multishot: Strzelasz większą ilością piłeczek naraz.

[ D ] Damage: Twoje ataki są silniejsze (szybciej niszczą puszki).

[ R ] Rapid Fire: Strzelasz znacznie szybciej.

Instalacja i Uruchomienie
Pobierz kod lub sklonuj repozytorium.

Upewnij się, że w folderze z plikiem wykonywalnym (.exe) znajduje się folder "assets" zawierający pliki graficzne:

gracz.png

kebab.png

mandat.png

Grafiki piw (kuflowe.png, perla.png, itd.)

Uruchom plik wykonywalny.

Technologie
Język: C++

Silnik: Raylib

Grafika: Pixel Perfect (skalowanie punktowe)


