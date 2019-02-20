#pragma once

#include <ceres/ceres.h>
#include <Eigen/Core>

#include "multirotor_sim/satellite.h"
#include "multirotor_sim/wsg84.h"

#include "factors/shield.h"


using namespace Eigen;

class PseudorangeFunctor
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    PseudorangeFunctor()
    {
      active_ = false;
    }

    void init(const GTime& _t, const Vector2d& _rho, const Satellite& sat, const Vector3d& _rec_pos_ecef, const Matrix2d& cov, const double& sweight)
    {
        // We don't have ephemeris for this satellite, we can't do anything with it yet
        if (sat.eph_.A == 0)
            return;

        t = _t;
        rho = _rho;
        rec_pos = _rec_pos_ecef;
        sat.computePositionVelocityClock(t, sat_pos, sat_vel, sat_clk_bias);

        // Earth rotation correction. The change in velocity can be neglected.
        Vector3d los_to_sat = sat_pos - rec_pos;
        double tau = los_to_sat.norm() / Satellite::C_LIGHT;
        sat_pos -= sat_vel * tau;
        double xrot = sat_pos.x() + sat_pos.y() * Satellite::OMEGA_EARTH * tau;
        double yrot = sat_pos.y() - sat_pos.x() * Satellite::OMEGA_EARTH * tau;
        sat_pos.x() = xrot;
        sat_pos.y() = yrot;

        los_to_sat = sat_pos - rec_pos;
        Vector2d az_el = sat.los2azimuthElevation(rec_pos, los_to_sat);
        ion_delay = sat.ionosphericDelay(t, WSG84::ecef2lla(rec_pos), az_el);
        Xi_ = cov.inverse().llt().matrixL().transpose();
        active_ = true;
    }

    template <typename T>
    bool operator()(const T* _x, const T* _v, const T* _clk, const T* _x_e2n, const T* _s, T* _res) const
    {
        typedef Matrix<T,3,1> Vec3;
        typedef Matrix<T,2,1> Vec2;


        Xform<T> x(_x);
        Map<const Vec3> v_b(_v);
        Map<const Vec2> clk(_clk);
        Xform<T> x_e2n(_x_e2n);
        Map<Vec2> res(_res);
        T s = (*_s);
        if (s > 1.0)
            s = (T)1.0;
        else if (s < 0.0)
            s = (T)0.0;


        Vec3 v_ECEF = x_e2n.q().rota(x.q().rota(v_b));
        Vec3 p_ECEF = x_e2n.transforma(x.t());
        Vec3 los_to_sat = sat_pos - p_ECEF;
        T los_dist = los_to_sat.norm();

        Vec2 rho_hat;
        rho_hat(0) = los_to_sat.norm() + ion_delay + (T)Satellite::C_LIGHT*(clk(0)- sat_clk_bias(0));
        rho_hat(1) = (sat_vel - v_ECEF).dot(los_to_sat.normalized()) + (T)Satellite::C_LIGHT*(clk(1) - sat_clk_bias(1));

        res = s * (rho - rho_hat);
        _res[2] = sw * (s - 1.0);

        /// TODO: Check if time or rec_pos have deviated too much and re-calculate ion_delay and earth rotation effect

        return true;
    }
    bool active_ = false;
    GTime t;
    Vector2d rho;
    Vector3d sat_pos;
    Vector3d sat_vel;
    Vector2d sat_clk_bias;
    double ion_delay;
    Vector3d rec_pos;
    Matrix2d Xi_;
    double sw;
};

typedef ceres::AutoDiffCostFunction<FunctorShield<PseudorangeFunctor>, 3, 7, 3, 2, 7, 1> PseudorangeFactorAD;