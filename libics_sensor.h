/*
 * libics: Image Cytometry Standard file reading and writing.
 *
 * Copyright (C) 2000-2013, 2016 Cris Luengo and others
 * Copyright 2015, 2016:
 *   Scientific Volume Imaging Holding B.V. Laapersveld 63,
 *   1213 VB Hilversum
 *   [Scientific Volume Imaging Holding BV](http://www.svi.nl)
 *
 * Contact: libics@svi.nl
 *
 * Large chunks of this library written by
 *    Bert Gijsbers
 *    Dr. Hans T.M. van der Voort
 * And also Damir Sudar, Geert van Kempen, Jan Jitze Krol,
 * Chiel Baarslag and Fons Laan.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * FILE : libics_sensor.h
 * Written by Bert Gijsbers, Scientific Volume Imaging BV, Hilversum, NL
 *
 * Access functions to ICS sensor parameters.
 */

#ifndef LIBICS_SENSOR_H
#define LIBICS_SENSOR_H

#ifndef LIBICS_H
#include "libics.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

ICSEXPORT Ics_Error IcsEnableWriteSensor (ICS* ics, int enable);
/* This function enables writing the sensor parameters to disk. */

ICSEXPORT char const* IcsGetSensorType (ICS const* ics, int channel);
/* Get the sensor type string. */

ICSEXPORT Ics_Error IcsSetSensorType (ICS* ics, int channel,
                                          char const* sensor_type);
/* Set the sensor type string. */

ICSEXPORT char const* IcsGetSensorModel (ICS const* ics);
/* Get the sensor model string. */

ICSEXPORT Ics_Error IcsSetSensorModel (ICS* ics, char const* sensor_model);
/* Set the sensor model string. */

ICSEXPORT int IcsGetSensorChannels (ICS const* ics);
/* Get the number of sensor channels. */

ICSEXPORT Ics_Error IcsSetSensorChannels (ICS* ics, int channels);
/* Set the number of sensor channels. */

ICSEXPORT double IcsGetSensorPinholeRadius (ICS const* ics, int channel);
/* Get the pinhole radius for a sensor channel. */

ICSEXPORT Ics_Error IcsSetSensorPinholeRadius (ICS* ics, int channel, double radius);
/* Set the pinhole radius for a sensor channel. */

ICSEXPORT double IcsGetSensorExcitationWavelength (ICS const* ics, int channel);
/* Get the excitation wavelength for a sensor channel. */

ICSEXPORT Ics_Error IcsSetSensorExcitationWavelength (ICS* ics, int channel, double wl);
/* Set the excitation wavelength for a sensor channel. */

ICSEXPORT double IcsGetSensorEmissionWavelength (ICS const* ics, int channel);
/* Get the emission wavelength for a sensor channel. */

ICSEXPORT Ics_Error IcsSetSensorEmissionWavelength (ICS* ics, int channel, double wl);
/* Set the emission wavelength for a sensor channel. */

ICSEXPORT int IcsGetSensorPhotonCount (ICS const* ics, int channel);
/* Get the excitation photon count for a sensor channel. */

ICSEXPORT Ics_Error IcsSetSensorPhotonCount (ICS* ics, int channel, int cnt);
/* Set the excitation photon count for a sensor channel. */

ICSEXPORT double IcsGetSensorMediumRI (ICS const* ics);
/* Get the sensor embedding medium refractive index. */

ICSEXPORT Ics_Error IcsSetSensorMediumRI (ICS* ics, double ri);
/* Set the sensor embedding medium refractive index. */

ICSEXPORT double IcsGetSensorLensRI (ICS const* ics);
/* Get the sensor design medium refractive index. */

ICSEXPORT Ics_Error IcsSetSensorLensRI (ICS* ics, double ri);
/* Set the sensor design medium refractive index. */

ICSEXPORT double IcsGetSensorNumAperture (ICS const* ics);
/* Get the sensor numerical apperture */

ICSEXPORT Ics_Error IcsSetSensorNumAperture (ICS* ics, double na);
/* Set the sensor numerical apperture */

ICSEXPORT double IcsGetSensorPinholeSpacing (ICS const* ics);
/* Get the sensor Nipkow Disk pinhole spacing. */

ICSEXPORT Ics_Error IcsSetSensorPinholeSpacing (ICS* ics, double spacing);
/* Set the sensor Nipkow Disk pinhole spacing. */

ICSEXPORT char const* IcsGetSensorSTEDDepletionMode (ICS const* ics, int channel);
/* Get the STED depletion mode. */

ICSEXPORT Ics_Error IcsSetSensorSTEDDepletionMode(ICS* ics, int channel,
                                                  char const* sted_mode);
/* Set the STED depletion mode */

ICSEXPORT double IcsGetSensorSTEDLambda (ICS const* ics, int channel);
/* Get the STED inhibition wavelength */
    
ICSEXPORT Ics_Error IcsSetSensorSTEDLambda(ICS* ics, int channel,
                                           double lambda);
/* Set the STED inhibition wavelength */

ICSEXPORT double IcsGetSensorSTEDSatFactor(ICS const* ics, int channel);
/* Get the STED saturation factor */

ICSEXPORT Ics_Error IcsSetSensorSTEDSatFactor(ICS* ics, int channel,
                                              double factor);
/* Set the STED saturation factor */

ICSEXPORT double IcsGetSensorSTEDImmFraction(ICS const* ics, int channel);
/* Get the fraction that is not inhibited by STED */
    
ICSEXPORT Ics_Error IcsSetSensorSTEDImmFraction(ICS* ics, int channel,
                                                double fraction);
/* Set the fraction that is not inhibited by STED */

ICSEXPORT double IcsGetSensorSTEDVPPM(ICS const* ics, int channel);
/* Get the STED  vortex to phase plate mix fraction */
    
ICSEXPORT Ics_Error IcsSetSensorSTEDVPPM(ICS* ics, int channel, double vppm);
/* Set the STED vortex to phase plate mix fraction */
    
/* Get the Detector ppu per channel. */
double IcsGetSensorDetectorPPU (ICS const* ics, int channel);

/* Set the Detector ppu per channel. */
Ics_Error IcsSetSensorDetectorPPU (ICS* ics, int channel, double ppu);

/* Get the Detector baseline per channel. */
double IcsGetSensorDetectorBaseline (ICS const* ics, int channel);

/* Set the Detector baseline per channel. */
Ics_Error IcsSetSensorDetectorBaseline (ICS* ics, int channel, double baseline);

/* Get the Detector lineAvgCnt per channel. */
double IcsGetSensorDetectorLineAvgCnt (ICS const* ics, int channel);

/* Set the Detector lineAvgCnt per channel. */
Ics_Error IcsSetSensorDetectorLineAvgCnt (ICS* ics, int channel, double lineAvgCnt);

#ifdef __cplusplus
}
#endif

#endif /* LIBICS_INTERN_H */
