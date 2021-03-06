//
//  Inductance.c
//  BlueBookFunctions
//
//  Created by PeterCoolAssHuber on 2017-03-29.
//  Copyright © 2017 Peter Huber. All rights reserved.
//

// This is a port of Swift-based code. It has been optimized on the iMac platform for speed and accuracy, but note that the code itself is somewhat verbose to aid in debugging.

#include "Inductance.h"

#define BBF_CONVERGENCE_ITERATIONS  (200)

double SelfInductance(const PCH_CoilSection aSection)
{
    // Check to make sure that r1<r2 and z1<z2 for aSection and if not, fix tthe values
    PCH_CoilSection theSection = aSection;
    
    if (theSection.r1 > theSection.r2)
    {
        double newR1 = theSection.r2;
        theSection.r2 = theSection.r1;
        theSection.r1 = newR1;
    }
    
    if (theSection.z1 > theSection.z2)
    {
        double newZ1 = theSection.z2;
        theSection.z2 = theSection.z1;
        theSection.z1 = newZ1;
    }
    
    double sectionArea = (theSection.r2 - theSection.r1) * (theSection.z2 - theSection.z1);
    double I1 = theSection.J * sectionArea / theSection.N;
    
    double N1 = theSection.N;
    
    double result = (M_PI * MU0 * N1 * N1 / (6.0 * BBF_WINDHT_FACTOR * theSection.windowHt)) * (gsl_pow_2(theSection.r2 + theSection.r1) + 2.0 * gsl_pow_2(theSection.r1));
    
    double multiplier = M_PI * MU0 * BBF_WINDHT_FACTOR * theSection.windowHt * N1 * N1 / gsl_pow_2(N1 * I1);
    
    double runningSum = 0.0;
    
    for (int i=0; i<BBF_CONVERGENCE_ITERATIONS; i++)
    {
        double n = i + 1.0;
        
        double m = n * M_PI / (BBF_WINDHT_FACTOR * theSection.windowHt);
        
        double x1 = m * theSection.r1;
        double x2 = m * theSection.r2;
        double xc = m * theSection.coreRadius;
        
        double scaledFn = ScaledF(n, theSection);
        double exponent = 2.0 * (xc - x1);
        double scaledTK1 = ScaledIntegralOfTK1(x1, x2);
        
        double scaledPartOfI1 = 0.0;
        double unscaledPartOfI1 = PartialScaledIntegralOfTL1(x1, x2, &scaledPartOfI1);
        
        double mult = E(n, theSection) - M_PI_2;
        
        double newWay = mult * exp(x1) * scaledPartOfI1 - M_PI_2 *  unscaledPartOfI1 + exp(exponent) * (scaledFn * scaledTK1);
        
        runningSum += multiplier * (gsl_pow_2(J(n, theSection)) / gsl_pow_4(m)) * newWay;

    }
    
    result += runningSum;
    
    return result;
}

double MutualInductance(const PCH_CoilSection aFromSection, const PCH_CoilSection aToSection)
{
    // Check to make sure that the two coil sections have the correct order for r1, r2, z1, and z2
    
    PCH_CoilSection fromSection = aFromSection;
    PCH_CoilSection toSection = aToSection;
    
    if (fromSection.r1 > fromSection.r2)
    {
        double newR1 = fromSection.r2;
        fromSection.r2 = fromSection.r1;
        fromSection.r1 = newR1;
    }
    
    if (fromSection.z1 > fromSection.z2)
    {
        double newZ1 = fromSection.z2;
        fromSection.z2 = fromSection.z1;
        fromSection.z1 = newZ1;
    }
    
    if (toSection.r1 > toSection.r2)
    {
        double newR1 = toSection.r2;
        toSection.r2 = toSection.r1;
        toSection.r1 = newR1;
    }
    
    if (toSection.z1 > toSection.z2)
    {
        double newZ1 = toSection.z2;
        toSection.z2 = toSection.z1;
        toSection.z1 = newZ1;
    }
    
    bool isSameRadialPosition = fabs(aFromSection.r1 - aToSection.r1) < 0.001;
    
    // Coil sections in the same radial position must have the 'from' section lower than the 'to' section. Coil sections that are not in the same radial position must have the 'from' section closer to the core than the 'to' section.
    
    if (isSameRadialPosition)
    {
        if (fromSection.z1 > toSection.z1)
        {
            PCH_CoilSection newFrom = toSection;
            toSection = fromSection;
            fromSection = newFrom;
        }
    }
    else
    {
        if (fromSection.r1 > toSection.r1)
        {
            PCH_CoilSection newFrom = toSection;
            toSection = fromSection;
            fromSection = newFrom;
        }
    }
    
    double sectionArea1 = (fromSection.r2 - fromSection.r1) * (fromSection.z2 - fromSection.z1);
    double I1 = fromSection.J * sectionArea1 / fromSection.N;
    double sectionArea2 = (toSection.r2 - toSection.r1) * (toSection.z2 - toSection.z1);
    double I2 = toSection.J * sectionArea2 / toSection.N;
    
    double N1 = fromSection.N;
    double N2 = toSection.N;
    
    double r1 = fromSection.r1;
    double r2 = fromSection.r2;
    double r3 = toSection.r1;
    double r4 = toSection.r2;
    double rc = fromSection.coreRadius;
    
    double result = 0.0;
    
    if (isSameRadialPosition)
    {
        result = (M_PI * MU0 * N1 * N2 / (6.0 * BBF_WINDHT_FACTOR * fromSection.windowHt)) * (gsl_pow_2(r2 + r1) + 2.0 * gsl_pow_2(r1));
    }
    else
    {
        result = (M_PI * MU0 * N1 * N2 / (3.0 * BBF_WINDHT_FACTOR * fromSection.windowHt)) * (gsl_pow_2(r1) + r1 * r2 + gsl_pow_2(r2));
    }
    
    double multiplier = M_PI * MU0 * BBF_WINDHT_FACTOR * fromSection.windowHt * N1 * N2 / ((N1 * I1) * (N2 * I2));
    
    double runningSum = 0.0;
    
    for (int i=0; i<BBF_CONVERGENCE_ITERATIONS; i++)
    {
        double n = i + 1.0;
        
        double m = n * M_PI / (BBF_WINDHT_FACTOR * fromSection.windowHt);
        
        double x1 = m * r1;
        double x2 = m * r2;
        double x3 = m * r3;
        double x4 = m * r4;
        double xc = m * rc;
        
        if (isSameRadialPosition)
        {
            double scaledFn = ScaledF(n, fromSection);
            double exponent = 2.0 * (xc - x1);
            double scaledTK1 = ScaledIntegralOfTK1(x1, x2);
            
            double scaledPartOfI1 = 0.0;
            double unscaledPartOfI1 = PartialScaledIntegralOfTL1(x1, x2, &scaledPartOfI1);
            
            double mult = E(n, fromSection) - M_PI_2;
            
            double newWay = mult * exp(x1) * scaledPartOfI1 - M_PI_2 *  unscaledPartOfI1 + exp(exponent) * (scaledFn * scaledTK1);
            
            runningSum += multiplier * ((J(n, fromSection) * J(n, toSection)) / gsl_pow_4(m)) * newWay;
        }
        else
        {
            double outerExp = exp(2.0 * xc - x3 - x1);
            double innerExp = exp(-2.0 * xc + 2.0 * x1);
            
            double firstProduct = ScaledIntegralOfTK1(x3, x4) * ScaledIntegralOfTI1(x1, x2);
            double secondProduct = ScaledD(n, toSection) * ScaledIntegralOfTK1(x1, x2);
            
            double insideTerm = innerExp * firstProduct + secondProduct;
            double newWay = outerExp * insideTerm;
            
            runningSum += multiplier * ((J(n, fromSection) * J(n, toSection)) / gsl_pow_4(m)) * newWay;
        }
    }
    
    result += runningSum;
    
    return result;
}
