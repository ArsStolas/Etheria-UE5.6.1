/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "FModularSurfaceAudioModule" - Source
 */

#include "Modules/ModuleManager.h"

class FModularSurfaceAudioModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FModularSurfaceAudioModule, ModularSurfaceAudio)
