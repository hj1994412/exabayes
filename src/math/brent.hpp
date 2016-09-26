#include <vector>

namespace brent {
///////////////////////////////////////////////////////////////////////////////
//                                 FUNC BASE                                 //
///////////////////////////////////////////////////////////////////////////////
class func_base
{
    ///////////////////////////////////////////////////////////////////////////
    //                            PUBLIC INTERFACE                           //
    ///////////////////////////////////////////////////////////////////////////
public:
    // ________________________________________________________________________
    virtual ~func_base(){}
    // ________________________________________________________________________
    virtual double                    operator()(
        double) = 0;
};


///////////////////////////////////////////////////////////////////////////////
//                                 MONIC POLY                                //
///////////////////////////////////////////////////////////////////////////////
class monicPoly : public func_base
{
    ///////////////////////////////////////////////////////////////////////////
    //                              PUBLIC DATA                              //
    ///////////////////////////////////////////////////////////////////////////
public:
    std::vector<double> coeff;

    ///////////////////////////////////////////////////////////////////////////
    //                            PUBLIC INTERFACE                           //
    ///////////////////////////////////////////////////////////////////////////
public:
    // ________________________________________________________________________
    virtual ~monicPoly(){}
    // ________________________________________________________________________
    virtual double                    operator()(
        double x);
    // ________________________________________________________________________
    monicPoly(
        const size_t degree)
        : coeff(degree)
    {}
    // ________________________________________________________________________
    monicPoly(
        const std::vector<double>& v)
        : coeff(v)
    {}
    // ________________________________________________________________________
    monicPoly(
        const double* c,
        size_t        degree)
        : coeff(std::vector<double>(c, c + degree)){}
};


///////////////////////////////////////////////////////////////////////////////
//                                    POLY                                   //
///////////////////////////////////////////////////////////////////////////////
class Poly : public func_base
{
    ///////////////////////////////////////////////////////////////////////////
    //                              PUBLIC DATA                              //
    ///////////////////////////////////////////////////////////////////////////
public:
    std::vector<double> coeff;        // a vector of size nterms i.e. 1+degree

    ///////////////////////////////////////////////////////////////////////////
    //                            PUBLIC INTERFACE                           //
    ///////////////////////////////////////////////////////////////////////////
public:
    // ________________________________________________________________________
    virtual ~Poly(){}
    // ________________________________________________________________________
    virtual double                    operator()(
        double x);
    // ________________________________________________________________________
    Poly(
        const size_t degree)
        : coeff(1 + degree)
    {}
    // ________________________________________________________________________
    Poly(
        const std::vector<double>& v)
        : coeff(v)
    {}
    // ________________________________________________________________________
    Poly(
        const double* c,
        size_t        degree)
        : coeff(std::vector<double>(c, 1 + c + degree)){}
};


// ____________________________________________________________________________
double                                glomin(
    double     a,
    double     b,
    double     c,
    double     m,
    double     e,
    double     t,
    func_base& f,
    double&    x);

// ____________________________________________________________________________
double                                local_min(
    double     a,
    double     b,
    double     t,
    func_base& f,
    double&    x);

// ____________________________________________________________________________
double                                local_min_rc(
    double&a,
    double&b,
    int&   status,
    double value);

// ____________________________________________________________________________
double                                r8_abs(
    double x);

// ____________________________________________________________________________
double                                r8_epsilon();

// ____________________________________________________________________________
double                                r8_max(
    double x,
    double y);

// ____________________________________________________________________________
double                                r8_sign(
    double x);

// ____________________________________________________________________________
void                                  timestamp();

// ____________________________________________________________________________
double                                zero(
    double     a,
    double     b,
    double     t,
    func_base& f);

// ____________________________________________________________________________
void                                  zero_rc(
    double a,
    double b,
    double t,
    double&arg,
    int&   status,
    double value);

// === simple wrapper functions
// === for convenience and/or compatibility
// ____________________________________________________________________________
double glomin(double a, double b, double c, double m, double e, double t,
    double f(double x), double&x);
// ____________________________________________________________________________
double local_min(double a, double b, double t, double f(double x),
    double&x);
// ____________________________________________________________________________
double zero(double a, double b, double t, double f(double x));
}
