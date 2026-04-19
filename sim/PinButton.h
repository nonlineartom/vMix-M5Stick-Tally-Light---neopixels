// Shim for MultiButton's PinButton when building for Wokwi.
// -I sim ahead of the device MultiButton library; identical public API so
// c_MAIN.ino (`PinButton btnM5(37);`) compiles unchanged.
//
// In the sim build, the Wokwi diagram wires the M5 button to GPIO35 rather
// than GPIO37 (not broken out on the DevKitC board model). A_GLOBAL.ino
// defines BTN_M5_PIN / BTN_ACTION_PIN which handle that remap — we still
// accept whatever pin the constructor is given.
#ifndef SIM_PINBUTTON_H
#define SIM_PINBUTTON_H

#include <Arduino.h>

class PinButton {
public:
  explicit PinButton(int pin) : _pin(pin) {
    pinMode(_pin, INPUT);  // external pull-ups in the diagram
  }

  void update() {
    const uint32_t now = millis();
    const bool pressed = (digitalRead(_pin) == LOW);

    // debounce
    if (pressed != _lastRaw) {
      _lastChange = now;
      _lastRaw = pressed;
    }
    if (now - _lastChange < 25) return;

    if (pressed && !_held) {
      _held = true;
      _pressStart = now;
      _clickCandidate = true;
    } else if (!pressed && _held) {
      _held = false;
      const uint32_t dur = now - _pressStart;

      if (dur >= 900) {
        _longClick = true;
        _clickCandidate = false;
        _pendingClick = false;
      } else if (_clickCandidate) {
        if (_pendingClick && (now - _firstClickAt) < 350) {
          _doubleClick = true;
          _pendingClick = false;
        } else {
          _pendingClick = true;
          _firstClickAt = now;
        }
        _clickCandidate = false;
      }
    }

    // commit the single click if the double-click window has lapsed
    if (_pendingClick && (now - _firstClickAt) >= 350) {
      _singleClick = true;
      _pendingClick = false;
    }
  }

  bool isClick()       { bool v = _singleClick; _singleClick = false; return v; }
  bool isSingleClick() { return isClick(); }
  bool isDoubleClick() { bool v = _doubleClick; _doubleClick = false; return v; }
  bool isLongClick()   { bool v = _longClick;   _longClick   = false; return v; }
  bool isPressed()     { return _held; }

private:
  int _pin;
  bool _lastRaw = false;
  bool _held = false;
  uint32_t _lastChange = 0;
  uint32_t _pressStart = 0;

  bool _clickCandidate = false;
  bool _pendingClick = false;
  uint32_t _firstClickAt = 0;

  bool _singleClick = false;
  bool _doubleClick = false;
  bool _longClick = false;
};

#endif
