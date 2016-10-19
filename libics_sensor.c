/*
 * libics: Image Cytometry Standard file reading and writing.
 *
 * Copyright (C) 2000-2013, 2016 Cris Luengo and others
 * Copyright 2015, 2016:
 *   Scientific Volume Imaging Holding B.V. Laapersveld 63,
 *   1213 VB Hilversum
 *   [Scientific Volume Imaging Holding BV](http://www.svi.nl)
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


/*
 * This function enables writing the sensor parameters to disk.
 */
Ics_Error IcsEnableWriteSensor (ICS* ics, int enable)
{
    ICS_FM_WMD( ics );
    ics->WriteSensor = (enable ? 1 : 0);
    return IcsErr_Ok;
}

/*
 * Get the sensor type string of a sensor channel.
 */
char const* IcsGetSensorType (ICS const* ics, int channel)
{
    return ics->Type[channel];
}

/*
 * Set the sensor type string for a sensor channel.
 */
Ics_Error IcsSetSensorType (ICS* ics, int channel,
                            char const* sensor_type)
{
    ICS_FM_WMD( ics );
    IcsStrCpy (ics->Type[channel], sensor_type, sizeof(ics->Type[channel]));
    return IcsErr_Ok;
}

/*
 * Get the sensor model string.
 */
char const* IcsGetSensorModel (ICS const* ics)
{
    return ics->Model;
}

/*
 * Set the sensor model string.
 */
Ics_Error IcsSetSensorModel (ICS* ics, char const* sensor_model)
{
    ICS_FM_WMD( ics );
    IcsStrCpy (ics->Model, sensor_model, sizeof(ics->Model));
    return IcsErr_Ok;
}

/*
 * Get the number of sensor channels.
 */
int IcsGetSensorChannels (ICS const* ics)
{
    return ics->SensorChannels;
}

/*
 * Set the number of sensor channels.
 */
Ics_Error IcsSetSensorChannels (ICS* ics, int channels)
{
    ICS_FM_WMD( ics );
    ICSTR( channels < 0 || channels > ICS_MAX_LAMBDA, IcsErr_NotValidAction );
    ics->SensorChannels = channels;
    return IcsErr_Ok;
}

/*
 * Get the pinhole radius for a sensor channel.
 */
double IcsGetSensorPinholeRadius (ICS const* ics, int channel)
{
    if (channel < 0 || channel >= ics->SensorChannels)
        return 0;
    else
        return ics->PinholeRadius[channel];
}

/*
 * Set the pinhole radius for a sensor channel.
 */
Ics_Error IcsSetSensorPinholeRadius (ICS* ics, int channel, double radius)
{
    ICS_FM_WMD( ics );
    ICSTR( channel < 0 || channel >= ics->SensorChannels, IcsErr_NotValidAction );
    ics->PinholeRadius[channel] = radius;
    return IcsErr_Ok;
}

/*
 * Get the excitation wavelength for a sensor channel.
 */
double IcsGetSensorExcitationWavelength (ICS const* ics, int channel)
{
    if (channel < 0 || channel >= ics->SensorChannels)
        return 0;
    else
        return ics->LambdaEx[channel];
}

/*
 * Set the excitation wavelength for a sensor channel.
 */
Ics_Error IcsSetSensorExcitationWavelength (ICS* ics, int channel, double wl)
{
    ICS_FM_WMD( ics );
    ICSTR( channel < 0 || channel >= ics->SensorChannels, IcsErr_NotValidAction );
    ics->LambdaEx[channel] = wl;
    return IcsErr_Ok;
}

/*
 * Get the emission wavelength for a sensor channel.
 */
double IcsGetSensorEmissionWavelength (ICS const* ics, int channel)
{
    if (channel < 0 || channel >= ics->SensorChannels)
        return 0;
    else
        return ics->LambdaEm[channel];
}

/*
 * Set the emission wavelength for a sensor channel.
 */
Ics_Error IcsSetSensorEmissionWavelength (ICS* ics, int channel, double wl)
{
    ICS_FM_WMD( ics );
    ICSTR( channel < 0 || channel >= ics->SensorChannels, IcsErr_NotValidAction );
    ics->LambdaEm[channel] = wl;
    return IcsErr_Ok;
}

/*
 * Get the excitation photon count for a sensor channel.
 */
int IcsGetSensorPhotonCount (ICS const* ics, int channel)
{
    if (channel < 0 || channel >= ics->SensorChannels)
        return 0;
    else
        return ics->ExPhotonCnt[channel];
}

/*
 * Set the excitation photon count for a sensor channel.
 */
Ics_Error IcsSetSensorPhotonCount (ICS* ics, int channel, int cnt)
{
    ICS_FM_WMD( ics );
    ICSTR( channel < 0 || channel >= ics->SensorChannels, IcsErr_NotValidAction );
    ics->ExPhotonCnt[channel] = cnt;
    return IcsErr_Ok;
}

/*
 * Get the sensor embedding medium refractive index.
 */
double IcsGetSensorMediumRI (ICS const* ics)
{
    return ics->RefrInxMedium;
}

/*
 * Set the sensor embedding medium refractive index.
 */
Ics_Error IcsSetSensorMediumRI (ICS* ics, double ri)
{
    ICS_FM_WMD( ics );
    ics->RefrInxMedium = ri;
    return IcsErr_Ok;
}

/*
 * Get the sensor design medium refractive index.
 */
double IcsGetSensorLensRI (ICS const* ics)
{
    return ics->RefrInxLensMedium;
}

/*
 * Set the sensor design medium refractive index.
 */
Ics_Error IcsSetSensorLensRI (ICS* ics, double ri)
{
    ICS_FM_WMD( ics );
    ics->RefrInxLensMedium = ri;
    return IcsErr_Ok;
}

/*
 * Get the sensor numerical apperture
 */
double IcsGetSensorNumAperture (ICS const* ics)
{
    return ics->NumAperture;
}

/*
 * Set the sensor numerical apperture
 */
Ics_Error IcsSetSensorNumAperture (ICS* ics, double na)
{
    ICS_FM_WMD( ics );
    ics->NumAperture = na;
    return IcsErr_Ok;
}

/*
 * Get the sensor Nipkow Disk pinhole spacing.
 */
double IcsGetSensorPinholeSpacing (ICS const* ics)
{
    return ics->PinholeSpacing;
}

/*
 * Set the sensor Nipkow Disk pinhole spacing.
 */
Ics_Error IcsSetSensorPinholeSpacing (ICS* ics, double spacing)
{
    ICS_FM_WMD( ics );
    ics->PinholeSpacing = spacing;
    return IcsErr_Ok;
}

/*
 * Get the STED mode per channel.
 */
char const*  IcsGetSensorSTEDDepletionMode(ICS const* ics, int channel)
{
    if (channel < 0 || channel >= ics->SensorChannels)
        return 0;
    else
        return ics->StedDepletionMode[channel];
}

/*
 * Set the STED depletion mode per channel.
 */
Ics_Error IcsSetSensorSTEDDepletionMode(ICS* ics, int channel,
                         char const* depletion_mode)
{
    ICS_FM_WMD( ics );
    ICSTR( channel < 0 || channel >= ics->SensorChannels, IcsErr_NotValidAction );
    IcsStrCpy (ics->StedDepletionMode[channel], depletion_mode,
               sizeof(ics->StedDepletionMode[channel]));
    return IcsErr_Ok;
}

/*
 * Get the STED inhibition wavelength per channel.
 */
double IcsGetSensorSTEDLambda (ICS const* ics, int channel)
{
    if (channel < 0 || channel >= ics->SensorChannels)
        return 0;
    else
        return ics->StedLambda[channel];
}

    /*
     * Set the STED inhibition wavelength per channel.
     */
Ics_Error IcsSetSensorSTEDLambda (ICS* ics, int channel, double lambda)
{
    ICS_FM_WMD( ics );
    ICSTR( channel < 0 || channel >= ics->SensorChannels, IcsErr_NotValidAction );
    ics->StedLambda[channel] = lambda;
        return IcsErr_Ok;
}

/*
 * Get the STED saturation factor per channel.
 */
double IcsGetSensorSTEDSatFactor (ICS const* ics, int channel)
{
    if (channel < 0 || channel >= ics->SensorChannels)
        return 0;
    else
        return ics->StedSatFactor[channel];
}

/*
 * Set the STED saturation factor per channel.
 */
Ics_Error IcsSetSensorSTEDSatFactor (ICS* ics, int channel, double factor)
{
    ICS_FM_WMD( ics );
    ICSTR( channel < 0 || channel >= ics->SensorChannels, IcsErr_NotValidAction );
    ics->StedSatFactor[channel] = factor;
    return IcsErr_Ok;
}

    /*
     * Get the STED immunity fraction per channel.
     */
double IcsGetSensorSTEDImmFraction (ICS const* ics, int channel)
{
    if (channel < 0 || channel >= ics->SensorChannels)
        return 0;
    else
        return ics->StedImmFraction[channel];
}

    /*
     * Set the STED immunity fraction per channel.
     */
Ics_Error IcsSetSensorSTEDImmFraction (ICS* ics, int channel, double fraction)
{
    ICS_FM_WMD( ics );
    ICSTR( channel < 0 || channel >= ics->SensorChannels, IcsErr_NotValidAction );
    ics->StedImmFraction[channel] = fraction;
        return IcsErr_Ok;
}

    /*
     * Get the STED vortex to phase plate mix per channel.
     */
double IcsGetSensorSTEDVPPM (ICS const* ics, int channel)
{
    if (channel < 0 || channel >= ics->SensorChannels)
        return 0;
    else
        return ics->StedVPPM[channel];
}

    /*
     * Set the STED vortex to phase plate mix per channel.
     */
Ics_Error IcsSetSensorSTEDVPPM (ICS* ics, int channel, double vppm)
{
    ICS_FM_WMD( ics );

    ICSTR( channel < 0 || channel >= ics->SensorChannels, IcsErr_NotValidAction );
    ics->StedVPPM[channel] = vppm;
    return IcsErr_Ok;
}


    /*
     * Get the Detector ppu per channel.
     */
double IcsGetSensorDetectorPPU (ICS const* ics, int channel)
{
    if (channel < 0 || channel >= ics->SensorChannels)
        return 0;
    else
        return ics->DetectorPPU[channel];
}

    /*
     * Set the Detector ppu per channel.
     */
Ics_Error IcsSetSensorDetectorPPU (ICS* ics, int channel, double ppu)
{
    ICS_FM_WMD( ics );

    ICSTR( channel < 0 || channel >= ics->SensorChannels, IcsErr_NotValidAction );
    ics->DetectorPPU[channel] = ppu;
    return IcsErr_Ok;
}

    /*
     * Get the Detector baseline per channel.
     */
double IcsGetSensorDetectorBaseline (ICS const* ics, int channel)
{
    if (channel < 0 || channel >= ics->SensorChannels)
        return 0;
    else
        return ics->DetectorBaseline[channel];
}

    /*
     * Set the Detector baseline per channel.
     */
Ics_Error IcsSetSensorDetectorBaseline (ICS* ics, int channel, double baseline)
{
    ICS_FM_WMD( ics );

    ICSTR( channel < 0 || channel >= ics->SensorChannels, IcsErr_NotValidAction );
    ics->DetectorBaseline [channel] = baseline;
    return IcsErr_Ok;
}

    /*
     * Get the Detector lineAvgCnt per channel.
     */
double IcsGetSensorDetectorLineAvgCnt (ICS const* ics, int channel)
{
    if (channel < 0 || channel >= ics->SensorChannels)
        return 0;
    else
        return ics->DetectorLineAvgCnt[channel];
}

    /*
     * Set the Detector lineAvgCnt per channel.
     */
Ics_Error IcsSetSensorDetectorLineAvgCnt (ICS* ics, int channel, double lineAvgCnt)
{
    ICS_FM_WMD( ics );

    ICSTR( channel < 0 || channel >= ics->SensorChannels, IcsErr_NotValidAction );
    ics->DetectorLineAvgCnt[channel] = lineAvgCnt;
    return IcsErr_Ok;
}
