# PSMove-Clicker

This program was primarly made for ADOFAI, but you can use it as a mouse.

Btw I'm not responsible for breaking your PSMove

## How do I use it?

I recommend using [PSMoveServiceEx](https://github.com/Timocop/PSMoveServiceEx/) instead of [PSMoveService](https://github.com/psmoveservice/PSMoveService), but you can also use it.

Run PSMoveService first, then PSMove Clicker.

Circle button - Right Mouse Button  
Select button - Escape Button  
PS Button - Sets main controller in ADOFAI mode

## How do I click?

You will see...  
HINT: Rage.
 
## Not so fun facts

This repository has:

- ugly code
- probably memory leaks, but who cares anyways
- bad ui
- polish language (TODO - english translation)

Oh and it only (somehow) works on Windows

## How to build

### MSVC

Paste the binaries from in the source folder and also the dll file to your exe folder  
Or build PSMoveService yourself  
Or download build of PSMoveService, then generate .lib definition file

It should look like this:

```text
PSMove Clicker
│   main.cpp
│   PSMoveClient_CAPI.dll
│   PSMoveClient_CAPI.lib
```

```text
x64
└───Release
        PSMove Clicker.exe
        PSMoveClient_CAPI.dll
```

Also keep the NadWin directory (it's a GUI library from stone age)

### Other compilers

Good luck
