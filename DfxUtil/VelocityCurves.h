#pragma once
#include <vector>

namespace dfx
{
    // "Knee-based" curves:
    // kneePos is the 0 to 1 knee position, where kneePos < 0.5
    // means use concave curve, kneePos = 0.5 means use linear
    // curve, and kneePos > 0.5 means use convex curve.

    class KneeCurve {
    public:

        std::vector<double> pts;
        double outputOffset;
        double outputFullScale;
        double kneePos;

    public:

        KneeCurve(int n, double outputOffset_, double outputFullScale_, double kneePos_);
        KneeCurve(const KneeCurve& src);
        KneeCurve(KneeCurve&& src) noexcept;

        virtual ~KneeCurve() = default;

        void operator=(const KneeCurve& src);
        void operator=(KneeCurve&& src) noexcept;

        double CurvePoint(double relVel);

        void Generate(double kneePos_);
    };

    // Curves based on notion of dynamic range and audio decibels

    class DynRangeCurve {
    public:

        std::vector<double> pts;
        double dB; // Stored just because

    protected:

        double ComputePt(double v);

    public:

        DynRangeCurve(double dB_, int npts_);
        DynRangeCurve(const DynRangeCurve& src);
        DynRangeCurve(DynRangeCurve&& src) noexcept;

        void operator=(const DynRangeCurve& src);
        void operator=(DynRangeCurve&& src) noexcept;

        void Generate(double dB_);
    };

}