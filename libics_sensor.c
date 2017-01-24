/*
 * libics: Image Cytometry Standard file reading and writing.
 *
 * Copyright 2015-2017:
 *   Scientific Volume Imaging Holding B.V.
 *   Laapersveld 63, 1213 VB Hilversum, The Netherlands
 *   https://www.svi.nl
 *
 * Copyright (C) 2000-2013 Cris Luengo and others
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
 * FILE : libics_sensor.c
 * Written by Bert Gijsbers, Scientific Volume Imaging BV, Hilversum, NL
 *
 * The following library functions are contained in this file:
 *
 *   IcsEnableWriteSensor
 *   IcsSetSensorType
 *   IcsSetSensorModel
 *   IcsSetSensorChannels
 *   IcsSetSensorPinholeRadius
 *   IcsSetSensorExcitationWavelength
 *   IcsSetSensorEmissionWavelength
 *   IcsSetSensorPhotonCount
 *   IcsSetSensorMediumRI
 *   IcsSetSensorLensRI
 *   IcsSetSensorNumAperture
 *   IcsSetSensorPinholeSpacing
 *   IcsSetSTEDMode
 *   IcsSetSTEDLambda
 *   IcsSetSTEDSaturationFactor
 *   IcsSetSTEDImmunityFactor
 *   IcsSetSTEDVortexToPhasePlateMix
 *   IcsSetDetectorPPU
 *   IcsSetDetectorBaseline
 *   IcsSetDetectorLineAvgCnt
 */


#include <stdlib.h>
#include <string.h>
#include "libics_sensor.h"
#include "libics_intern.h"


/* This function enables writing the sensor parameters to disk. */
Ics_Error IcsEnableWriteSensor(ICS *ics,
                               int  enable)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    ics->writeSensor = enable ? 1 : 0;
    return IcsErr_Ok;
}


/* Get the sensor type string of a sensor channel. */
char const* IcsGetSensorType(const ICS *ics,
                             int        channel)
{
    return ics->type[channel];
}


/* Set the sensor type string for a sensor channel. */
Ics_Error IcsSetSensorType(ICS        *ics,
                           int         channel,
                           const char *sensorType)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    IcsStrCpy(ics->type[channel], sensorType, sizeof(ics->type[channel]));
    return IcsErr_Ok;
}


/* Get the sensor model string. */
const char *IcsGetSensorModel(const ICS *ics)
{
    return ics->model;
}


/* Set the sensor model string. */
Ics_Error IcsSetSensorModel(ICS        *ics,
                            const char *sensorModel)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    IcsStrCpy(ics->model, sensorModel, sizeof(ics->model));
    return IcsErr_Ok;
}


/* Get the number of sensor channels. */
int IcsGetSensorChannels(const ICS *ics)
{
    return ics->sensorChannels;
}


/* Set the number of sensor channels. */
Ics_Error IcsSetSensorChannels(ICS *ics,
                               int  channels)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    if (channels < 0 || channels > ICS_MAX_LAMBDA)
        return IcsErr_NotValidAction;
    ics->sensorChannels = channels;
    return IcsErr_Ok;
}


/* Get the pinhole radius for a sensor channel. */
double IcsGetSensorPinholeRadius(const ICS *ics,
                                 int        channel)
{
    if (channel < 0 || channel >= ics->sensorChannels)
        return 0;
    else
        return ics->pinholeRadius[channel];
}


/* Set the pinhole radius for a sensor channel. */
Ics_Error IcsSetSensorPinholeRadius(ICS    *ics,
                                    int     channel,
                                    double  radius)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    if (channel < 0 || channel >= ics->sensorChannels)
        return IcsErr_NotValidAction;
    ics->pinholeRadius[channel] = radius;
    return IcsErr_Ok;
}


/* Get the excitation wavelength for a sensor channel. */
double IcsGetSensorExcitationWavelength(const ICS *ics,
                                        int        channel)
{
    if (channel < 0 || channel >= ics->sensorChannels)
        return 0;
    else
        return ics->lambdaEx[channel];
}


/* Set the excitation wavelength for a sensor channel. */
Ics_Error IcsSetSensorExcitationWavelength(ICS    *ics,
                                           int     channel,
                                           double  wl)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    if (channel < 0 || channel >= ics->sensorChannels)
        return IcsErr_NotValidAction;
    ics->lambdaEx[channel] = wl;
    return IcsErr_Ok;
}


/* Get the emission wavelength for a sensor channel. */
double IcsGetSensorEmissionWavelength(const ICS *ics,
                                      int        channel)
{
    if (channel < 0 || channel >= ics->sensorChannels)
        return 0;
    else
        return ics->lambdaEm[channel];
}


/* Set the emission wavelength for a sensor channel. */
Ics_Error IcsSetSensorEmissionWavelength(ICS    *ics,
                                         int     channel,
                                         double  wl)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    if (channel < 0 || channel >= ics->sensorChannels)
        return IcsErr_NotValidAction;
    ics->lambdaEm[channel] = wl;
    return IcsErr_Ok;
}


/* Get the excitation photon count for a sensor channel. */
int IcsGetSensorPhotonCount(const ICS *ics,
                            int        channel)
{
    if (channel < 0 || channel >= ics->sensorChannels)
        return 0;
    else
        return ics->exPhotonCnt[channel];
}


/* Set the excitation photon count for a sensor channel. */
Ics_Error IcsSetSensorPhotonCount(ICS *ics,
                                  int  channel,
                                  int  cnt)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    if (channel < 0 || channel >= ics->sensorChannels)
        return IcsErr_NotValidAction;
    ics->exPhotonCnt[channel] = cnt;
    return IcsErr_Ok;
}


/* Get the sensor embedding medium refractive index. */
double IcsGetSensorMediumRI(const ICS *ics)
{
    return ics->refrInxMedium;
}


/* Set the sensor embedding medium refractive index. */
Ics_Error IcsSetSensorMediumRI(ICS    *ics,
                               double  ri)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    ics->refrInxMedium = ri;
    return IcsErr_Ok;
}


/* Get the sensor design medium refractive index. */
double IcsGetSensorLensRI(const ICS *ics)
{
    return ics->refrInxLensMedium;
}


/* Set the sensor design medium refractive index. */
Ics_Error IcsSetSensorLensRI(ICS    *ics,
                             double  ri)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    ics->refrInxLensMedium = ri;
    return IcsErr_Ok;
}


/* Get the sensor numerical apperture */
double IcsGetSensorNumAperture(const ICS *ics)
{
    return ics->numAperture;
}


/* Set the sensor numerical apperture */
Ics_Error IcsSetSensorNumAperture(ICS    *ics,
                                  double  na)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    ics->numAperture = na;
    return IcsErr_Ok;
}


/* Get the sensor Nipkow Disk pinhole spacing. */
double IcsGetSensorPinholeSpacing(const ICS *ics)
{
    return ics->pinholeSpacing;
}


/* Set the sensor Nipkow Disk pinhole spacing. */
Ics_Error IcsSetSensorPinholeSpacing(ICS    *ics,
                                     double  spacing)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    ics->pinholeSpacing = spacing;
    return IcsErr_Ok;
}


/* Get the STED mode per channel. */
const char * IcsGetSensorSTEDDepletionMode(const ICS *ics,
                                           int        channel)
{
    if (channel < 0 || channel >= ics->sensorChannels)
        return 0;
    else
        return ics->stedDepletionMode[channel];
}


/* Set the STED depletion mode per channel. */
Ics_Error IcsSetSensorSTEDDepletionMode(ICS        *ics,
                                        int         channel,
                                        const char *depletionMode)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    if (channel < 0 || channel >= ics->sensorChannels)
        return IcsErr_NotValidAction;
    IcsStrCpy(ics->stedDepletionMode[channel], depletionMode,
              sizeof(ics->stedDepletionMode[channel]));
    return IcsErr_Ok;
}


/* Get the STED inhibition wavelength per channel. */
double IcsGetSensorSTEDLambda(const ICS *ics,
                              int        channel)
{
    if (channel < 0 || channel >= ics->sensorChannels)
        return 0;
    else
        return ics->stedLambda[channel];
}


/* Set the STED inhibition wavelength per channel. */
Ics_Error IcsSetSensorSTEDLambda(ICS    *ics,
                                 int     channel,
                                 double  lambda)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    if (channel < 0 || channel >= ics->sensorChannels)
        return IcsErr_NotValidAction;
    ics->stedLambda[channel] = lambda;
    return IcsErr_Ok;
}


/* Get the STED saturation factor per channel. */
double IcsGetSensorSTEDSatFactor(const ICS *ics,
                                 int        channel)
{
    if (channel < 0 || channel >= ics->sensorChannels)
        return 0;
    else
        return ics->stedSatFactor[channel];
}


/* Set the STED saturation factor per channel. */
Ics_Error IcsSetSensorSTEDSatFactor(ICS    *ics,
                                    int     channel,
                                    double  factor)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    if (channel < 0 || channel >= ics->sensorChannels)
        return IcsErr_NotValidAction;
    ics->stedSatFactor[channel] = factor;
    return IcsErr_Ok;
}


/* Get the STED immunity fraction per channel. */
double IcsGetSensorSTEDImmFraction(const ICS *ics,
                                   int        channel)
{
    if (channel < 0 || channel >= ics->sensorChannels)
        return 0;
    else
        return ics->stedImmFraction[channel];
}


/* Set the STED immunity fraction per channel. */
Ics_Error IcsSetSensorSTEDImmFraction(ICS    *ics,
                                      int     channel,
                                      double  fraction)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    if (channel < 0 || channel >= ics->sensorChannels)
        return IcsErr_NotValidAction;
    ics->stedImmFraction[channel] = fraction;
    return IcsErr_Ok;
}


/* Get the STED vortex to phase plate mix per channel. */
double IcsGetSensorSTEDVPPM(const ICS *ics,
                            int        channel)
{
    if (channel < 0 || channel >= ics->sensorChannels)
        return 0;
    else
        return ics->stedVPPM[channel];
}


/* Set the STED vortex to phase plate mix per channel. */
Ics_Error IcsSetSensorSTEDVPPM(ICS    *ics,
                               int     channel,
                               double  vppm)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    if (channel < 0 || channel >= ics->sensorChannels)
        return IcsErr_NotValidAction;
    ics->stedVPPM[channel] = vppm;
    return IcsErr_Ok;
}



/* Get the Detector ppu per channel. */
double IcsGetSensorDetectorPPU(const ICS *ics,
                               int        channel)
{
    if (channel < 0 || channel >= ics->sensorChannels)
        return 0;
    else
        return ics->detectorPPU[channel];
}


/* Set the Detector ppu per channel. */
Ics_Error IcsSetSensorDetectorPPU(ICS    *ics,
                                  int     channel,
                                  double  ppu)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    if (channel < 0 || channel >= ics->sensorChannels)
        return IcsErr_NotValidAction;
    ics->detectorPPU[channel] = ppu;
    return IcsErr_Ok;
}


/* Get the Detector baseline per channel. */
double IcsGetSensorDetectorBaseline(const ICS *ics,
                                    int        channel)
{
    if (channel < 0 || channel >= ics->sensorChannels)
        return 0;
    else
        return ics->detectorBaseline[channel];
}


/* Set the Detector baseline per channel. */
Ics_Error IcsSetSensorDetectorBaseline(ICS    *ics,
                                       int     channel,
                                       double  baseline)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    if (channel < 0 || channel >= ics->sensorChannels)
        return IcsErr_NotValidAction;
    ics->detectorBaseline [channel] = baseline;
    return IcsErr_Ok;
}


/* Get the Detector lineAvgCnt per channel. */
double IcsGetSensorDetectorLineAvgCnt(const ICS *ics,
 int channel)
{
    if (channel < 0 || channel >= ics->sensorChannels)
        return 0;
    else
        return ics->detectorLineAvgCnt[channel];
}


 /* Set the Detector lineAvgCnt per channel. */
Ics_Error IcsSetSensorDetectorLineAvgCnt(ICS    *ics,
                                         int     channel,
                                         double  lineAvgCnt)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    if (channel < 0 || channel >= ics->sensorChannels)
        return IcsErr_NotValidAction;
    ics->detectorLineAvgCnt[channel] = lineAvgCnt;
    return IcsErr_Ok;
}


/* Get the state of a sensor parameter. */
ICSEXPORT Ics_SensorState IcsGetSensorState(ICS                 *ics,
                                            Ics_SensorParameter  parameter,
                                            int                  channel)
{
    switch (parameter) {
        case ICS_SENSOR_PINHOLE_RADIUS:
            return ics->pinholeRadiusState[channel];
            break;
        case ICS_SENSOR_LAMBDA_EXCITATION:
            return ics->lambdaExState[channel];
            break;
        case ICS_SENSOR_LAMBDA_EMISSION:
            return ics->lambdaEmState[channel];
            break;
        case ICS_SENSOR_PHOTON_COUNT:
            return ics->exPhotonCntState[channel];
            break;
        case ICS_SENSOR_MEDIUM_REFRACTIVE_INDEX:
            return ics->refrInxMediumState;
            break;
        case ICS_SENSOR_NUMERICAL_APERTURE:
            return ics->numApertureState;
            break;
        case ICS_SENSOR_LENS_REFRACTIVE_INDEX:
            return ics->refrInxLensMediumState;
            break;
        case ICS_SENSOR_PINHOLE_SPACING:
            return ics->pinholeSpacingState;
            break;
        case ICS_SENSOR_STED_DEPLETION_MODE:
            return ics->stedDepletionModeState[channel];
            break;
        case ICS_SENSOR_STED_LAMBDA:
            return ics->stedLambdaState[channel];
            break;
        case ICS_SENSOR_STED_SATURATION_FACTOR:
            return ics->stedSatFactorState[channel];
            break;
        case ICS_SENSOR_STED_IMM_FRACTION:
            return ics->stedImmFractionState[channel];
            break;
        case ICS_SENSOR_STED_VPPM:
            return ics->stedVPPMState[channel];
            break;
        case ICS_SENSOR_DETECTOR_PPU:
            return ics->detectorPPUState[channel];
            break;
        case ICS_SENSOR_DETECTOR_BASELINE:
            return ics->detectorBaselineState[channel];
            break;
        case ICS_SENSOR_DETECTOR_LINE_AVG_COUNT:
            return ics->detectorLineAvgCntState[channel];
            break;
        default:
            return IcsSensorState_default;
    }
}


/* Set the state of a sensor parameter. */
ICSEXPORT Ics_Error IcsSetSensorState(ICS                 *ics,
                                      Ics_SensorParameter  parameter,
                                      int                  channel,
                                      Ics_SensorState      state)
{
    if ((ics == NULL) || (ics->fileMode == IcsFileMode_read))
        return IcsErr_NotValidAction;

    switch (parameter) {
        case ICS_SENSOR_PINHOLE_RADIUS:
        case ICS_SENSOR_LAMBDA_EXCITATION:
        case ICS_SENSOR_LAMBDA_EMISSION:
        case ICS_SENSOR_PHOTON_COUNT:
        case ICS_SENSOR_STED_DEPLETION_MODE:
        case ICS_SENSOR_STED_LAMBDA:
        case ICS_SENSOR_STED_SATURATION_FACTOR:
        case ICS_SENSOR_STED_IMM_FRACTION:
        case ICS_SENSOR_STED_VPPM:
        case ICS_SENSOR_DETECTOR_PPU:
        case ICS_SENSOR_DETECTOR_BASELINE:
        case ICS_SENSOR_DETECTOR_LINE_AVG_COUNT:
            if (channel < 0 || channel >= ics->sensorChannels)
                return IcsErr_NotValidAction;
            break;
        default:
            break;
    }

    switch (parameter) {
        case ICS_SENSOR_PINHOLE_RADIUS:
            ics->pinholeRadiusState[channel] = state;
            break;
        case ICS_SENSOR_LAMBDA_EXCITATION:
            ics->lambdaExState[channel] = state;
            break;
        case ICS_SENSOR_LAMBDA_EMISSION:
            ics->lambdaEmState[channel] = state;
            break;
        case ICS_SENSOR_PHOTON_COUNT:
            ics->exPhotonCntState[channel] = state;
            break;
        case ICS_SENSOR_MEDIUM_REFRACTIVE_INDEX:
            ics->refrInxMediumState = state;
            break;
        case ICS_SENSOR_NUMERICAL_APERTURE:
            ics->numApertureState = state;
            break;
        case ICS_SENSOR_LENS_REFRACTIVE_INDEX:
            ics->refrInxLensMediumState = state;
            break;
        case ICS_SENSOR_PINHOLE_SPACING:
            ics->pinholeSpacingState = state;
            break;
        case ICS_SENSOR_STED_DEPLETION_MODE:
            ics->stedDepletionModeState[channel] = state;
            break;
        case ICS_SENSOR_STED_LAMBDA:
            ics->stedLambdaState[channel] = state;
            break;
        case ICS_SENSOR_STED_SATURATION_FACTOR:
            ics->stedSatFactorState[channel] = state;
            break;
        case ICS_SENSOR_STED_IMM_FRACTION:
            ics->stedImmFractionState[channel] = state;
            break;
        case ICS_SENSOR_STED_VPPM:
            ics->stedVPPMState[channel] = state;
            break;
        case ICS_SENSOR_DETECTOR_PPU:
            ics->detectorPPUState[channel] = state;
            break;
        case ICS_SENSOR_DETECTOR_BASELINE:
            ics->detectorBaselineState[channel] = state;
            break;
        case ICS_SENSOR_DETECTOR_LINE_AVG_COUNT:
            ics->detectorLineAvgCntState[channel] = state;
            break;
        default:
            return IcsErr_NotValidAction;
    }

    return IcsErr_Ok;
}
