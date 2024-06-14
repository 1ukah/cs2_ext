An external hack for Counter-Strike 2 that reads the game's memory using Windows RPM and displays information using an ImGui overlay.

*The hack reads the game's memory to gather data and display it on an overlay created with ImGui. While this approach avoids writing or injecting data into the game, it is less secure as it involves directly opening a handle with the game. A safer alternative would involve hijacking another process.*

## Features

- Reads game memory using Windows RPM.
- Renders information using an ImGui overlay.
- Only reads memory, does not write to the game.
