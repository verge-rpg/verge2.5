
  VERGE
  Additions/Changes
  January 17, 2000

  Important

  - NOTE!!! user.cfg no longer pays attention to 'startmap'. you must
    make an 'autoexec' routine in system.vc and it'll call that.
  - I recommend only HookTimering routines that do not use local vars.
  - It has been brought to my attention that MikMod has memory leaks, and
    it will sometimes clash with the engine and either hang or crash,
    possible even demand a reboot. No help for it; I don't know any audio
    programming. If V2 does begin to crash inexplicably, set sound_device to
    3 in user.cfg first and see if that has any effect on the stability of
    your game. May go to Seal, or something else.

  Modification Log

  2.5 beta 5

  * Started hicolor routines; add 'hicolor' to user.cfg and all modes will
    attempt to set 16-bit (or 15, failing 16).

  2.5 beta 4
  
  * Fixed some timing issues w/ events
  * Fixed layer render bug introduced w/ new code
  * lastent is now set correctly when an event is called from within a
    movement script
  * Made execution halt if an AutoExec() was not found.
  * Calls to Render() will always clear the screen first.
  * Rewrote some more graphics code; RotScale() supports lucency as a result
  * Rewrote and tinkered with TMapLine/MapLine commands. Likely slower than
    before, but cleaner and safer; still work the same? TODO: pass map layer
    width/length to them internally so that running along extremities
    doesn't trunc on the left and wrap on the right. WARNING: because proper
    clipping isn't implemented yet, hitting the lower right of a map while
    performing wave effects or the like may crash the engine (theoretically
    speaking)

  2.5 beta 3

  * Rewrote a lot of graphics code internals; condensed many routines to
    eliminate some redundancy. Keep an eye out for anything suspicious, but
    should be okay.
  * Rewrote layer rendering routines and misc related routines; translucent
    layers act up sometimes? Not sure; keep an eye on this one too.

  2.5 ???

  * added 'numents'; returns total # of entities on the map
  * VERGE.EXE will now run without any other files. ^_^
  * Fixed HookKey
  * New input subsystem; won't lose keys when framerate drops in console
  * VESA 2.0 LFB code; any 256-color resolution if you've got it
  * New mouse routines; smooth now
  * New local variable and argument handling; == faster VC code
    NOTE!!! you *must* recompile all scripts with the new VCC for the new
    engine.
  * Unlimited strings
  * Rewrote console a bit; disabled browsetiles, needs to be rewritten
    Made console friendly to whatever font SYSTEM.FNT holds.
  * Made messaging and cpu_usage text friendly to font in SYSTEM.FNT.
  * Put some validity checking in a lot of areas
  * Rewrote VCC's preprocessor and rearranged a bit of the interpreter;
    A little more sensitive to duplicate naming; catches junk lingering in
    global area as an error now (helps with catching mismatched {}).
  * Me2 no longer crashes when you compile twice with VCC; this was due to
    PMODEW, now everything uses DOS4GW, and likely soon it'll go to Causeway.
  * Now V2 runs in pure-DOS without GPFing (geeze ;P)

  aen, <aen@verge-rpg.com>
