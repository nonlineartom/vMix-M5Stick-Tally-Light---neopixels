#ifndef K_PLUGINMANAGER_H
#define K_PLUGINMANAGER_H

//#include "plugins/LED_HAT.h";

class PluginManager {
  public:
    PluginManager(){}

    //M5TallyLEDHat ledHat;
    void onLive(){
      //ledHat.onLive();
    }

    void onPre() {
      //ledHat.onPre();
    }

    void onSafe() {
      //ledHat.onSafe();
    }
};

#endif
