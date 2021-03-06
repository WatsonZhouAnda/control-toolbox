/**********************************************************************************************************************
This file is part of the Control Toolbox (https://github.com/ethz-adrl/control-toolbox), copyright by ETH Zurich
Licensed under the BSD-2 license (see LICENSE file in main directory)
**********************************************************************************************************************/

#pragma once

#define IKFAST_HAS_LIBRARY
#define IKFAST_NAMESPACE hya_ik
#include <ikfast.h>
#include <ct/core/core.h>
#include <ct/rbd/state/JointState.h>
#include <ct/models/HyA/HyAJointLimits.h>

#include <ct/rbd/robot/kinematics/InverseKinematicsBase.h>

using namespace ikfast;

namespace ct {
namespace rbd {

template <typename SCALAR = double>
class HyAInverseKinematics : public InverseKinematicsBase<6, SCALAR>
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    using BASE = InverseKinematicsBase<6, SCALAR>;
    using JointPosition_t = typename BASE::JointPosition_t;
    using JointPositionsVector_t = typename BASE::JointPositionsVector_t;
    using RigidBodyPoseTpl = typename BASE::RigidBodyPoseTpl;

    HyAInverseKinematics() = default;

    ~HyAInverseKinematics() = default;

    bool computeInverseKinematics(JointPositionsVector_t& res,
        const RigidBodyPoseTpl& eeBasePose,
        const std::vector<size_t>& freeJoints = std::vector<size_t>()) override
    {
        res.clear();
        IkSolutionList<double> solutions;

        if (size_t(hya_ik::GetNumFreeParameters()) != freeJoints.size())
            throw std::runtime_error("Error specifying free joints in HyAInverseKinematics");

        // Data needs to be in row-major form.
        Eigen::Matrix<SCALAR, 3, 3, Eigen::RowMajor> eeBaseRotationRowMajor =
            eeBasePose.getRotationMatrix().toImplementation();
        const std::vector<double> freeJoints_ikf(freeJoints.begin(), freeJoints.end());
        hya_ik::ComputeIk(eeBasePose.position().toImplementation().data(), eeBaseRotationRowMajor.data(),
            freeJoints_ikf.size() > 0 ? freeJoints_ikf.data() : nullptr, solutions);

        size_t num_solutions = solutions.GetNumSolutions();

        if (num_solutions == 0)
            return false;  // no solution found

        // filter for joint limit violations
        JointState<6> sol;
        for (size_t i = 0; i < num_solutions; i++)
        {
            const IkSolutionBase<double>& solution = solutions.GetSolution(i);
            solution.GetSolution(
                sol.getPositions().data(), freeJoints_ikf.size() > 0 ? freeJoints_ikf.data() : nullptr);
            sol.toUniquePosition(ct::models::HyA::jointLowerLimit());
            if (sol.checkPositionLimits(ct::models::HyA::jointLowerLimit(), ct::models::HyA::jointUpperLimit()))
                res.push_back(sol.getPositions());
        }

        if(res.size() == 0)
        	return false; // no viable solution after filtering for joint limits

        return true;
    }

    bool computeInverseKinematics(JointPositionsVector_t& res,
        const RigidBodyPoseTpl& eeWorldPose,
        const RigidBodyPoseTpl& baseWorldPose,
        const std::vector<size_t>& freeJoints = std::vector<size_t>()) override
    {
        return computeInverseKinematics(res, eeWorldPose.inReferenceFrame(baseWorldPose), freeJoints);
    }
};
} /* namespace rbd */
} /* namespace ct */
