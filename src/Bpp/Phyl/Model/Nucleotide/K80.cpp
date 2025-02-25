// SPDX-FileCopyrightText: The Bio++ Development Group
//
// SPDX-License-Identifier: CECILL-2.1

#include "K80.h"

// From the STL:
#include <cmath>

using namespace bpp;

#include <Bpp/Numeric/Matrix/MatrixTools.h>

using namespace std;

/******************************************************************************/

K80::K80(shared_ptr<const NucleicAlphabet> alpha, double kappa) :
  AbstractParameterAliasable("K80."),
  AbstractReversibleNucleotideSubstitutionModel(alpha, make_shared<CanonicalStateMap>(alpha, false), "K80."),
  kappa_(kappa), r_(), l_(), k_(), exp1_(), exp2_(), p_(size_, size_)
{
  addParameter_(new Parameter("K80.kappa", kappa, Parameter::R_PLUS_STAR));
  updateMatrices_();
}

/******************************************************************************/

void K80::updateMatrices_()
{
  kappa_ = getParameterValue("kappa");
  k_ = (kappa_ + 1.) / 2.;
  r_ = isScalable() ? 4. / (kappa_ + 2.) : 4;

  // Frequences:
  freq_[0] = freq_[1] = freq_[2] = freq_[3] = 1. / 4.;

  // Generator:
  generator_(0, 0) = -2. - kappa_;
  generator_(1, 1) = -2. - kappa_;
  generator_(2, 2) = -2. - kappa_;
  generator_(3, 3) = -2. - kappa_;

  generator_(0, 1) = 1.;
  generator_(0, 3) = 1.;
  generator_(1, 0) = 1.;
  generator_(1, 2) = 1.;
  generator_(2, 1) = 1.;
  generator_(2, 3) = 1.;
  generator_(3, 0) = 1.;
  generator_(3, 2) = 1.;

  generator_(0, 2) = kappa_;
  generator_(1, 3) = kappa_;
  generator_(2, 0) = kappa_;
  generator_(3, 1) = kappa_;

  // Normalization:
  setScale(r_ / 4);

  // Exchangeability:
  exchangeability_ = generator_;
  MatrixTools::scale(exchangeability_, 4.);

  // Eigen values:
  eigenValues_[0] = 0;
  eigenValues_[1] = -r_ * (1. + kappa_) / 2;
  eigenValues_[2] = -r_ * (1. + kappa_) / 2;
  eigenValues_[3] = -r_;

  // Eigen vectors:
  leftEigenVectors_(0, 0) = 1. / 4.;
  leftEigenVectors_(0, 1) = 1. / 4.;
  leftEigenVectors_(0, 2) = 1. / 4.;
  leftEigenVectors_(0, 3) = 1. / 4.;
  leftEigenVectors_(1, 0) = 0.;
  leftEigenVectors_(1, 1) = 1. / 2.;
  leftEigenVectors_(1, 2) = 0.;
  leftEigenVectors_(1, 3) = -1. / 2.;
  leftEigenVectors_(2, 0) = 1. / 2.;
  leftEigenVectors_(2, 1) = 0.;
  leftEigenVectors_(2, 2) = -1. / 2.;
  leftEigenVectors_(2, 3) = 0.;
  leftEigenVectors_(3, 0) = 1. / 4.;
  leftEigenVectors_(3, 1) = -1. / 4.;
  leftEigenVectors_(3, 2) = 1. / 4.;
  leftEigenVectors_(3, 3) = -1. / 4.;

  rightEigenVectors_(0, 0) = 1.;
  rightEigenVectors_(0, 1) = 0.;
  rightEigenVectors_(0, 2) = 1.;
  rightEigenVectors_(0, 3) = 1.;
  rightEigenVectors_(1, 0) = 1.;
  rightEigenVectors_(1, 1) = 1.;
  rightEigenVectors_(1, 2) = 0.;
  rightEigenVectors_(1, 3) = -1.;
  rightEigenVectors_(2, 0) = 1.;
  rightEigenVectors_(2, 1) = 0.;
  rightEigenVectors_(2, 2) = -1.;
  rightEigenVectors_(2, 3) = 1.;
  rightEigenVectors_(3, 0) = 1.;
  rightEigenVectors_(3, 1) = -1.;
  rightEigenVectors_(3, 2) = 0;
  rightEigenVectors_(3, 3) = -1.;
}

/******************************************************************************/

double K80::Pij_t(size_t i, size_t j, double d) const
{
  l_ = rate_ * r_ * d;
  exp1_ = exp(-l_);
  exp2_ = exp(-k_ * l_);

  switch (i)
  {
  case 0: // A
    switch (j)
    {
    case 0: return 0.25 * (1. + exp1_) + 0.5 * exp2_; // A
    case 1: return 0.25 * (1. - exp1_);               // C
    case 2: return 0.25 * (1. + exp1_) - 0.5 * exp2_; // G
    case 3: return 0.25 * (1. - exp1_);               // T, U
    default: return 0;
    }
  case 1: // C
    switch (j)
    {
    case 0: return 0.25 * (1. - exp1_);               // A
    case 1: return 0.25 * (1. + exp1_) + 0.5 * exp2_; // C
    case 2: return 0.25 * (1. - exp1_);               // G
    case 3: return 0.25 * (1. + exp1_) - 0.5 * exp2_; // T, U
    default: return 0;
    }
  case 2: // G
    switch (j)
    {
    case 0: return 0.25 * (1. + exp1_) - 0.5 * exp2_; // A
    case 1: return 0.25 * (1. - exp1_);               // C
    case 2: return 0.25 * (1. + exp1_) + 0.5 * exp2_; // G
    case 3: return 0.25 * (1. - exp1_);               // T, U
    default: return 0;
    }
  case 3: // T, U
    switch (j)
    {
    case 0: return 0.25 * (1. - exp1_);               // A
    case 1: return 0.25 * (1. + exp1_) - 0.5 * exp2_; // C
    case 2: return 0.25 * (1. - exp1_);               // G
    case 3: return 0.25 * (1. + exp1_) + 0.5 * exp2_; // T, U
    default: return 0;
    }
  default: return 0;
  }
}

/******************************************************************************/

double K80::dPij_dt(size_t i, size_t j, double d) const
{
  l_ = rate_ * r_ * d;
  exp1_ = exp(-l_);
  exp2_ = exp(-k_ * l_);

  switch (i)
  {
  case 0: // A
    switch (j)
    {
    case 0: return rate_ * r_ / 4. * (-exp1_ - 2. * k_ * exp2_); // A
    case 1: return rate_ * r_ / 4. * (  exp1_);                   // C
    case 2: return rate_ * r_ / 4. * (-exp1_ + 2. * k_ * exp2_); // G
    case 3: return rate_ * r_ / 4. * (  exp1_);                   // T, U
    default: return 0;
    }
  case 1: // C
    switch (j)
    {
    case 0: return rate_ * r_ / 4. * (  exp1_);                   // A
    case 1: return rate_ * r_ / 4. * (-exp1_ - 2. * k_ * exp2_); // C
    case 2: return rate_ * r_ / 4. * (  exp1_);                   // G
    case 3: return rate_ * r_ / 4. * (-exp1_ + 2. * k_ * exp2_); // T, U
    default: return 0;
    }
  case 2: // G
    switch (j)
    {
    case 0: return rate_ * r_ / 4. * (-exp1_ + 2. * k_ * exp2_); // A
    case 1: return rate_ * r_ / 4. * (  exp1_);                   // C
    case 2: return rate_ * r_ / 4. * (-exp1_ - 2. * k_ * exp2_); // G
    case 3: return rate_ * r_ / 4. * (  exp1_);                   // T, U
    default: return 0;
    }
  case 3: // T, U
    switch (j)
    {
    case 0: return rate_ * r_ / 4. * (  exp1_);                   // A
    case 1: return rate_ * r_ / 4. * (-exp1_ + 2. * k_ * exp2_); // C
    case 2: return rate_ * r_ / 4. * (  exp1_);                   // G
    case 3: return rate_ * r_ / 4. * (-exp1_ - 2. * k_ * exp2_); // T, U
    default: return 0;
    }
  default: return 0;
  }
}

/******************************************************************************/

double K80::d2Pij_dt2(size_t i, size_t j, double d) const
{
  double k_2 = k_ * k_;
  double r_2 = rate_ * rate_ * r_ * r_;
  l_ = rate_ * r_ * d;
  exp1_ = exp(-l_);
  exp2_ = exp(-k_ * l_);

  switch (i)
  {
  case 0: // A
    switch (j)
    {
    case 0: return r_2 / 4. * (  exp1_ + 2. * k_2 * exp2_); // A
    case 1: return r_2 / 4. * (-exp1_);                    // C
    case 2: return r_2 / 4. * (  exp1_ - 2. * k_2 * exp2_); // G
    case 3: return r_2 / 4. * (-exp1_);                    // T, U
    default: return 0;
    }
  case 1: // C
    switch (j)
    {
    case 0: return r_2 / 4. * (-exp1_);                    // A
    case 1: return r_2 / 4. * (  exp1_ + 2. * k_2 * exp2_); // C
    case 2: return r_2 / 4. * (-exp1_);                    // G
    case 3: return r_2 / 4. * (  exp1_ - 2. * k_2 * exp2_); // T, U
    default: return 0;
    }
  case 2: // G
    switch (j)
    {
    case 0: return r_2 / 4. * (  exp1_ - 2. * k_2 * exp2_); // A
    case 1: return r_2 / 4. * (-exp1_);                    // C
    case 2: return r_2 / 4. * (  exp1_ + 2. * k_2 * exp2_); // G
    case 3: return r_2 / 4. * (-exp1_);                    // T, U
    default: return 0;
    }
  case 3: // T, U
    switch (j)
    {
    case 0: return r_2 / 4. * (-exp1_);                    // A
    case 1: return r_2 / 4. * (  exp1_ - 2. * k_2 * exp2_); // C
    case 2: return r_2 / 4. * (-exp1_);                    // G
    case 3: return r_2 / 4. * (  exp1_ + 2. * k_2 * exp2_); // T, U
    default: return 0;
    }
  default: return 0;
  }
  return 0;
}

/******************************************************************************/

const Matrix<double>& K80::getPij_t(double d) const
{
  l_ = rate_ * r_ * d;
  exp1_ = exp(-l_);
  exp2_ = exp(-k_ * l_);

  // A
  p_(0, 0) = 0.25 * (1. + exp1_) + 0.5 * exp2_; // A
  p_(0, 1) = 0.25 * (1. - exp1_);               // C
  p_(0, 2) = 0.25 * (1. + exp1_) - 0.5 * exp2_; // G
  p_(0, 3) = 0.25 * (1. - exp1_);               // T, U

  // C
  p_(1, 0) = 0.25 * (1. - exp1_);               // A
  p_(1, 1) = 0.25 * (1. + exp1_) + 0.5 * exp2_; // C
  p_(1, 2) = 0.25 * (1. - exp1_);               // G
  p_(1, 3) = 0.25 * (1. + exp1_) - 0.5 * exp2_; // T, U

  // G
  p_(2, 0) = 0.25 * (1. + exp1_) - 0.5 * exp2_; // A
  p_(2, 1) = 0.25 * (1. - exp1_);               // C
  p_(2, 2) = 0.25 * (1. + exp1_) + 0.5 * exp2_; // G
  p_(2, 3) = 0.25 * (1. - exp1_);               // T, U

  // T, U
  p_(3, 0) = 0.25 * (1. - exp1_);               // A
  p_(3, 1) = 0.25 * (1. + exp1_) - 0.5 * exp2_; // C
  p_(3, 2) = 0.25 * (1. - exp1_);               // G
  p_(3, 3) = 0.25 * (1. + exp1_) + 0.5 * exp2_; // T, U

  return p_;
}

const Matrix<double>& K80::getdPij_dt(double d) const
{
  l_ = rate_ * r_ * d;
  exp1_ = exp(-l_);
  exp2_ = exp(-k_ * l_);

  p_(0, 0) = rate_ * r_ / 4. * (-exp1_ - 2. * k_ * exp2_); // A
  p_(0, 1) = rate_ * r_ / 4. * (  exp1_);                   // C
  p_(0, 2) = rate_ * r_ / 4. * (-exp1_ + 2. * k_ * exp2_); // G
  p_(0, 3) = rate_ * r_ / 4. * (  exp1_);                   // T, U

  // C
  p_(1, 0) = rate_ * r_ / 4. * (  exp1_);                   // A
  p_(1, 1) = rate_ * r_ / 4. * (-exp1_ - 2. * k_ * exp2_); // C
  p_(1, 2) = rate_ * r_ / 4. * (  exp1_);                   // G
  p_(1, 3) = rate_ * r_ / 4. * (-exp1_ + 2. * k_ * exp2_); // T, U

  // G
  p_(2, 0) = rate_ * r_ / 4. * (-exp1_ + 2. * k_ * exp2_); // A
  p_(2, 1) = rate_ * r_ / 4. * (  exp1_);                   // C
  p_(2, 2) = rate_ * r_ / 4. * (-exp1_ - 2. * k_ * exp2_); // G
  p_(2, 3) = rate_ * r_ / 4. * (  exp1_);                   // T, U

  // T, U
  p_(3, 0) = rate_ * r_ / 4. * (  exp1_);                   // A
  p_(3, 1) = rate_ * r_ / 4. * (-exp1_ + 2. * k_ * exp2_); // C
  p_(3, 2) = rate_ * r_ / 4. * (  exp1_);                   // G
  p_(3, 3) = rate_ * r_ / 4. * (-exp1_ - 2. * k_ * exp2_); // T, U

  return p_;
}

const Matrix<double>& K80::getd2Pij_dt2(double d) const
{
  double k_2 = k_ * k_;
  double r_2 = rate_ * rate_ * r_ * r_;
  l_ = rate_ * r_ * d;
  exp1_ = exp(-l_);
  exp2_ = exp(-k_ * l_);

  p_(0, 0) = r_2 / 4. * (  exp1_ + 2. * k_2 * exp2_); // A
  p_(0, 1) = r_2 / 4. * (-exp1_);                    // C
  p_(0, 2) = r_2 / 4. * (  exp1_ - 2. * k_2 * exp2_); // G
  p_(0, 3) = r_2 / 4. * (-exp1_);                    // T, U

  // C
  p_(1, 0) = r_2 / 4. * (-exp1_);                    // A
  p_(1, 1) = r_2 / 4. * (  exp1_ + 2. * k_2 * exp2_); // C
  p_(1, 2) = r_2 / 4. * (-exp1_);                    // G
  p_(1, 3) = r_2 / 4. * (  exp1_ - 2. * k_2 * exp2_); // T, U

  // G
  p_(2, 0) = r_2 / 4. * (  exp1_ - 2. * k_2 * exp2_); // A
  p_(2, 1) = r_2 / 4. * (-exp1_);                    // C
  p_(2, 2) = r_2 / 4. * (  exp1_ + 2. * k_2 * exp2_); // G
  p_(2, 3) = r_2 / 4. * (-exp1_);                    // T, U

  // T, U
  p_(3, 0) = r_2 / 4. * (-exp1_);                    // A
  p_(3, 1) = r_2 / 4. * (  exp1_ - 2. * k_2 * exp2_); // C
  p_(3, 2) = r_2 / 4. * (-exp1_);                    // G
  p_(3, 3) = r_2 / 4. * (  exp1_ + 2. * k_2 * exp2_); // T, U

  return p_;
}

/******************************************************************************/
