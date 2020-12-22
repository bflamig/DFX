#include "VelocityCurves.h"

namespace dfx
{
    KneeCurve::KneeCurve(int n, double outputOffset_, double outputFullScale_, double kneePos_)
    : pts(n)
    , outputOffset(outputOffset_)
    , outputFullScale(outputFullScale_)
    , kneePos(kneePos_)
    {
        Generate(kneePos);
    }

    KneeCurve::KneeCurve(const KneeCurve& src)
    : pts(src.pts)
    , outputOffset(src.outputOffset)
    , outputFullScale(src.outputFullScale)
    , kneePos(src.kneePos)
    {
    }

    KneeCurve::KneeCurve(KneeCurve&& src) noexcept
    : pts(std::move(src.pts))
    , outputOffset(src.outputOffset)
    , outputFullScale(src.outputFullScale)
    , kneePos(src.kneePos)
    {
    }

    void KneeCurve::operator=(const KneeCurve& src)
    {
        if (&src != this)
        {
            pts = src.pts;
            outputOffset = src.outputOffset;
            outputFullScale = src.outputFullScale;
            kneePos = src.kneePos;
        }
    }

    void KneeCurve::operator=(KneeCurve&& src) noexcept
    {
        if (&src != this)
        {
            pts = std::move(src.pts);
            outputOffset = src.outputOffset;
            outputFullScale = src.outputFullScale;
            kneePos = src.kneePos;
        }
    }

    double KneeCurve::CurvePoint(double relVel)
    {
        // Here, relVel is linear velocity relative to full scale
        // kneePos is the 0 to 1 knee position, where kneePos < 0.5
        // means use concave curve, kneePos = 0.5 means use linear
        // curve, and kneePos > 0.5 means use convex curve.

        double sprime = outputFullScale - outputOffset;

        double a = (1.0 - 1.0 / kneePos);
        a *= a;
        double b = a == 1 ? relVel : ((pow(a, relVel) - 1.0) / (a - 1.0));
        b = b * sprime + outputOffset;
        b = ceil(b);
        if (b > outputFullScale) b = outputFullScale;
        //b = floor(b);

        return b;
    }

    void KneeCurve::Generate(double kneePos_)
    {
        kneePos = kneePos_;

        unsigned n = pts.size();
        double delta = 1.0 / (n - 1);

        double pt = 0.0;

        for (unsigned i = 0; i < n; i++)
        {
            double cp = CurvePoint(pt);
            pts[i] = cp;
            pt += delta;
        }
    }


    //
    // ////////////////////////////////////////////////////
    //

    DynRangeCurve::DynRangeCurve(double dB_, int npts_)
    : pts(npts_)
    , dB(dB_)
    {
        //pts.reserve(npts_);
        Generate(dB_);
    }

    DynRangeCurve::DynRangeCurve(const DynRangeCurve& src)
    : pts(src.pts)
    , dB(src.dB)
    {

    }

    DynRangeCurve::DynRangeCurve(DynRangeCurve&& src) noexcept
    : pts(std::move(src.pts))
    , dB(src.dB)
    {

    }

    void DynRangeCurve::operator=(const DynRangeCurve& src)
    {
        if (this != &src)
        {
            pts = src.pts;
            dB = src.dB;
        }
    }

    void DynRangeCurve::operator=(DynRangeCurve&& src) noexcept
    {
        if (this != &src)
        {
            pts = std::move(src.pts);
            dB = src.dB;
        }
    }

    double DynRangeCurve::ComputePt(double v)
    {
        double r = pow(10.0, dB / 20.0);
        double n = static_cast<double>(pts.size());
        double b = n / ((n - 1) * sqrt(r)) - 1.0 / (n - 1);
        double m = (1.0 - b) / n;
        double a = m * v + b;
        a *= a;

        return a;
    }

    void DynRangeCurve::Generate(double dB_)
    {
        dB = dB_;

        size_t n = pts.size();

        for (size_t i = 0; i < n; i++)
        {
            double vi = static_cast<double>(i + 1);
            double vo = ComputePt(vi);
            pts[i] = vo;
        }
    }

}