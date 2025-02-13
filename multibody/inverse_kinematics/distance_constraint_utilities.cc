#include "drake/multibody/inverse_kinematics/distance_constraint_utilities.h"

#include "drake/math/autodiff_gradient.h"

namespace drake {
namespace multibody {
namespace internal {
void CalcDistanceDerivatives(const MultibodyPlant<double>&,
                             const systems::Context<double>&,
                             const Frame<double>&, const Frame<double>&,
                             const Eigen::Vector3d&, double distance,
                             const Eigen::Vector3d&,
                             const Eigen::Ref<const AutoDiffVecXd>&,
                             double* distance_double) {
  *distance_double = distance;
}

void CalcDistanceDerivatives(const MultibodyPlant<double>& plant,
                             const systems::Context<double>& context,
                             const Frame<double>& frameA,
                             const Frame<double>& frameB,
                             const Eigen::Vector3d& p_ACa, double distance,
                             const Eigen::Vector3d& nhat_BA_W,
                             const Eigen::Ref<const AutoDiffVecXd>& q,
                             AutoDiffXd* distance_autodiff) {
  // Derivation to compute the gradient of the signed distance function w.r.t q:
  // The distance is
  // d = n̂_BA_Wᵀ * (p_WCa - p_WCb)             (1)
  //   = n̂_BA_Wᵀ * p_CbCa_W
  // where the Ca is the witness point on geometry A, and Cb is the witness
  // point on geometry B.
  // Hence when we take the gradient on both sides
  // ∂d / ∂q =   p_CbCa_Wᵀ * (∂n̂_BA_W / ∂q) + n̂_BA_Wᵀ * (∂p_CbCa_W /∂q)   (2)
  // Using the fact that p_CbCa_W = d * n̂_BA_W, we have
  // ∂d / ∂q = d * n̂_BA_Wᵀ * (∂n̂_BA_W / ∂q) + n̂_BA_Wᵀ * (∂p_CbCa_W /∂q)   (3)
  // We also know that for whatever q, the normal n̂_BA_W is always a unit length
  // vector, hence n̂_BA_Wᵀ * n̂_BA_W = 1 for whatever q. This invariance means
  // that the gradient of (n̂_BA_Wᵀ * n̂_BA_W) w.r.t q is zero, namely
  // ∂1 / ∂q = ∂(n̂_BA_Wᵀ * n̂_BA_W) / ∂q = 2 * n̂_BA_Wᵀ * (∂n̂_BA_W / ∂q)
  // Since ∂1 / ∂q = 0, we conclude that n̂_BA_Wᵀ * (∂n̂_BA_W / ∂q) = 0, hence the
  // first term in the right-hand side equation (3) disappears. Thus
  // ∂d / ∂q = n̂_BA_Wᵀ * (∂p_CbCa_W / ∂q)                                  (4)
  // Note that ∂p_CbCa_W / ∂q is computed as a Jacobian matrix in
  // MultibodyPlant.
  Eigen::Matrix<double, 3, Eigen::Dynamic> Jq_v_BCa_W(3, plant.num_positions());
  plant.CalcJacobianTranslationalVelocity(context, JacobianWrtVariable::kQDot,
                                          frameA, p_ACa, frameB,
                                          plant.world_frame(), &Jq_v_BCa_W);
  const Eigen::RowVectorXd ddistance_dq = nhat_BA_W.transpose() * Jq_v_BCa_W;
  distance_autodiff->value() = distance;
  distance_autodiff->derivatives() =
      ddistance_dq * math::autoDiffToGradientMatrix(q);
}

void CheckPlantIsConnectedToSceneGraph(
    const MultibodyPlant<double>& plant,
    const systems::Context<double>& plant_context) {
  if (!plant.geometry_source_is_registered()) {
    throw std::invalid_argument(
        "Kinematic constraint: MultibodyPlant has not registered "
        "with a SceneGraph yet. Please refer to "
        "AddMultibodyPlantSceneGraph on how to connect MultibodyPlant to "
        "SceneGraph.");
  }
  const auto& query_port = plant.get_geometry_query_input_port();
  if (!query_port.HasValue(plant_context)) {
    throw std::invalid_argument(
        "Kinematic constraint: Cannot get a valid "
        "geometry::QueryObject. Either the plant's geometry query input port "
        "is not properly connected to the SceneGraph's geometry query output "
        "port, or the plant_context_ is incorrect. Please refer to "
        "AddMultibodyPlantSceneGraph on connecting MultibodyPlant to "
        "SceneGraph.");
  }
}
}  // namespace internal
}  // namespace multibody
}  // namespace drake
