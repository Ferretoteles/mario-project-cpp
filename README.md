# LanternRun SFML - Classic Mario Style

Platformowka 2D w C++17 i SFML, przygotowana jako projekt do Qt Creatora.
Ta wersja skupia sie na czystym, klasycznym wygladzie Mario Bros: jasne niebo,
chmury, gory, krzaki przy ziemi, cegly, bloki ?, rury, monety, Goomba, Koopa,
Piranha Plant i duzy Bowser na ostatnim poziomie.

## Sterowanie

- A / D albo strzalki: ruch
- Shift: sprint
- Space / W / strzalka w gore: skok
- drugi skok w powietrzu: podwojny skok
- X albo C: fireball po zebraniu Fire Flower
- F2: edytor poziomu
- P: pauza
- Enter: start / wybor / dalej
- Escape: powrot albo zamkniecie

## Co jest w grze

- Mario/Luigi z czystego sprite sheetu `assets/sprites/mario_player_clean.png`.
- Goomba, Koopa, Piranha Plant i Bullet Bill z `assets/sprites/mario_enemies_clean.png`.
- Czyste bonusy PNG: moneta, gwiazdka, grzybek, klucz i serce w `assets/sprites/`.
- 5 dluzszych poziomow: laka, rury/sekrety, platformy, zamek z drzwiami na klucz i boss.
- Walka z Bowserem na poziomie 5, segmentowany pasek HP, napis BOSS i fazy 12-9, 8-5, 4-1 HP.
- Dodatkowe typy przeciwnikow: szybki Runner i twardszy Spiny.
- Double jump, sprint, zycia, monety, Fire Flower, Starman, grzybek, checkpointy i flaga.
- Ruchome platformy, trampoliny, kolce/lawa, rury, sekretne pokoje, klucze i zamkniete drzwi.
- Menu startowe, wybor postaci, wybor poziomu, sklep, pauza Resume/Restart/Exit, Game Over, Victory.
- HUD w prostym stylu platformowki: monety, zycia, world i czas.

## Assety

Wszystkie glowne sciezki do grafik sa w `src/AssetManager.cpp`.
Sprite sheety sa ladowane z:

- `assets/spritesheets/characters.png`
- `assets/spritesheets/enemies_bosses.png`
- `assets/spritesheets/items_objects_npcs.png`
- `assets/spritesheets/blocks.png`
- `assets/spritesheets/tileset.png`
- `assets/spritesheets/background_mountains.png`
- `assets/spritesheets/background_trees.png`
- `assets/spritesheets/background_clouds.png`
- `assets/sprites/mario_player_clean.png`
- `assets/sprites/mario_enemies_clean.png`
- `assets/sprites/bowser_clean.png`
- `assets/sprites/bowser_boss.png`
- `assets/sprites/mushroom.png`
- `assets/sprites/star.png`
- `assets/sprites/coin.png`
- `assets/sprites/key.png`

Mapowanie wycinkow sprite sheetow jest w `src/Sprites.cpp`.
Pixel art ma wylaczone wygladzanie tekstur przez `texture.setSmooth(false)`.
Sprite sheety postaci i przeciwnikow sa ladowane przez color key, wiec bohater
i wrogowie nie maja prostokatnego tla wokol siebie.

Gra uzywa `Tile = 32.0f`. Maly Mario ma docelowo 32x32 px na ekranie.

Tlo poziomu jest rysowane proceduralnie jako spokojny gradient z chmurami,
gorami i drzewami w parallax, bo arkusze tla mialy mocna kratke/palete testowa.
Kafelki sa czytelnym fallbackiem proceduralnym, zeby zly crop tilesetu nie psul gry.

## Gdzie zmieniac zasady

- Assety i sciezki: `src/AssetManager.cpp`
- Wycinki sprite sheetow i animacje: `src/Sprites.cpp`
- SpriteComponent: `src/SpriteComponent.cpp`
- Gra/state machine/menu: `src/Game.cpp`
- Gracz/double jump/fireball: `src/Player.cpp`
- Przeciwnicy i bossowie: `src/Enemy.cpp`
- Przedmioty: `src/Item.cpp`
- Poziomy/mapy/kafelki/tla: `src/Level.cpp`
- Zapis JSON: `src/SaveManager.cpp`
- Dzwieki i muzyka: `src/AudioManager.cpp`
- Questy: `src/QuestManager.cpp`
- Osiagniecia: `src/AchievementManager.cpp`

## Jak odpalic w Qt Creatorze

Najprostsza instrukcja krok po kroku jest w `START_TUTAJ.txt`.
