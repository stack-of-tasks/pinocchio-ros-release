//
// Copyright (c) 2020 LAAS-CNRS, INRIA
//

#include "pinocchio/algorithm/joint-configuration.hpp"
#include "pinocchio/algorithm/impulse-dynamics.hpp"
#include "pinocchio/algorithm/impulse-dynamics-derivatives.hpp"
#include "pinocchio/algorithm/cholesky.hpp"
#include "pinocchio/parsers/urdf.hpp"
#include "pinocchio/multibody/sample-models.hpp"

#include <iostream>

#include "pinocchio/utils/timer.hpp"

int main(int argc, const char ** argv)
{
  using namespace Eigen;
  using namespace pinocchio;

  PinocchioTicToc timer(PinocchioTicToc::US);
#ifdef NDEBUG
  const int NBT = 1000 * 100;
#else
  const int NBT = 1;
  std::cout << "(the time score in debug mode is not relevant) " << std::endl;
#endif

  // Build model
  Model model;

  std::string filename = PINOCCHIO_MODEL_DIR + std::string("/simple_humanoid.urdf");
  if (argc > 1)
    filename = argv[1];
  bool with_ff = true;

  if (argc > 2)
  {
    const std::string ff_option = argv[2];
    if (ff_option == "-no-ff")
      with_ff = false;
  }

  if (filename == "HS")
    buildModels::humanoidRandom(model, true);
  else if (with_ff)
    pinocchio::urdf::buildModel(filename, JointModelFreeFlyer(), model);
  //      pinocchio::urdf::buildModel(filename,JointModelRX(),model);
  else
    pinocchio::urdf::buildModel(filename, model);

  const std::string RA = "RARM_LINK6";
  const std::string LA = "LARM_LINK6";
  const std::string RF = "RLEG_LINK6";
  const std::string LF = "LLEG_LINK6";

  RigidConstraintModel ci_RF_6D(CONTACT_6D, model.getFrameId(RF), LOCAL);
  RigidConstraintData cd_RF_6D(ci_RF_6D);

  RigidConstraintModel ci_LF_6D(CONTACT_6D, model.getFrameId(LF), LOCAL);
  RigidConstraintData cd_LF_6D(ci_LF_6D);

  // Define contact infos structure
  const PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(RigidConstraintModel) contact_infos_empty;
  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(RigidConstraintData) contact_datas_empty;

  cholesky::ContactCholeskyDecomposition contact_chol_empty(model, contact_infos_empty);

  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(RigidConstraintModel) contact_infos_6D;
  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(RigidConstraintData) contact_datas_6D;
  contact_infos_6D.push_back(ci_RF_6D);
  contact_datas_6D.push_back(cd_RF_6D);

  cholesky::ContactCholeskyDecomposition contact_chol_6D(model, contact_infos_6D);

  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(RigidConstraintModel) contact_infos_6D6D;
  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(RigidConstraintData) contact_datas_6D6D;
  contact_infos_6D6D.push_back(ci_RF_6D);
  contact_datas_6D6D.push_back(cd_RF_6D);
  contact_infos_6D6D.push_back(ci_LF_6D);
  contact_datas_6D6D.push_back(cd_LF_6D);

  cholesky::ContactCholeskyDecomposition contact_chol_6D6D(model, contact_infos_6D6D);

  std::cout << "nq = " << model.nq << std::endl;
  std::cout << "nv = " << model.nv << std::endl;
  std::cout << "--" << std::endl;

  Data data(model);
  VectorXd qmax = Eigen::VectorXd::Ones(model.nq);

  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(VectorXd) qs(NBT);
  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(VectorXd) qdots(NBT);
  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(VectorXd) qddots(NBT);
  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(VectorXd) taus(NBT);

  for (size_t i = 0; i < NBT; ++i)
  {
    qs[i] = randomConfiguration(model, -qmax, qmax);
    qdots[i] = Eigen::VectorXd::Random(model.nv);
    qddots[i] = Eigen::VectorXd::Random(model.nv);
    taus[i] = Eigen::VectorXd::Random(model.nv);
  }

  double total_time = 0;
  initConstraintDynamics(model, data, contact_infos_empty);
  SMOOTH(NBT)
  {
    impulseDynamics(
      model, data, qs[_smooth], qdots[_smooth], contact_infos_empty, contact_datas_empty);
    timer.tic();
    computeImpulseDynamicsDerivatives(model, data, contact_infos_empty, contact_datas_empty);
    total_time += timer.toc(timer.DEFAULT_UNIT);
  }
  std::cout << "impulseDynamicsDerivs {} = \t\t" << (total_time / NBT) << std::endl;

  total_time = 0;
  initConstraintDynamics(model, data, contact_infos_6D);
  SMOOTH(NBT)
  {
    impulseDynamics(model, data, qs[_smooth], qdots[_smooth], contact_infos_6D, contact_datas_6D);
    timer.tic();
    computeImpulseDynamicsDerivatives(model, data, contact_infos_6D, contact_datas_6D);
    total_time += timer.toc(timer.DEFAULT_UNIT);
  }
  std::cout << "impulseDynamicsDerivs {6D} = \t\t" << (total_time / NBT) << std::endl;

  total_time = 0;
  initConstraintDynamics(model, data, contact_infos_6D6D);
  SMOOTH(NBT)
  {
    impulseDynamics(
      model, data, qs[_smooth], qdots[_smooth], contact_infos_6D6D, contact_datas_6D6D);
    timer.tic();
    computeImpulseDynamicsDerivatives(model, data, contact_infos_6D6D, contact_datas_6D6D);
    total_time += timer.toc(timer.DEFAULT_UNIT);
  }
  std::cout << "impulseDynamicsDerivs {6D,6D} = \t" << (total_time / NBT) << std::endl;
  return 0;
}
