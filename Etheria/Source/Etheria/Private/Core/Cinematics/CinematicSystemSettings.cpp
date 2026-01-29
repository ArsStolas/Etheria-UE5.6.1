/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UCinematicSystemSettings" - Source
 */
#include "Core/Cinematics/CinematicSystemSettings.h"

const UCinematicSystemSettings* UCinematicSystemSettings::Get()
{
    return GetDefault<UCinematicSystemSettings>();
}
