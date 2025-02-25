// SPDX-FileCopyrightText: The Bio++ Development Group
//
// SPDX-License-Identifier: CECILL-2.1

#include "../MixtureOfASubstitutionModel.h"
#include "RELAX.h"
#include "YN98.h"

#include <cmath>                     /* pow */

#include <Bpp/Numeric/NumConstants.h>
#include <Bpp/Numeric/Prob/SimpleDiscreteDistribution.h>

using namespace bpp;
using namespace std;

/******************************************************************************/

RELAX::RELAX(
    shared_ptr<const GeneticCode> gc,
    unique_ptr<CodonFrequencySetInterface> codonFreqs) :
  AbstractParameterAliasable("RELAX."),
  AbstractWrappedModel("RELAX."),
  AbstractWrappedTransitionModel("RELAX."),
  AbstractTotallyWrappedTransitionModel("RELAX."),
  AbstractBiblioTransitionModel("RELAX."),
  YNGP_M("RELAX.") // RELAX currenly inherits from YNGP_M as well, since it uses kappa and instead of the 5 GTR parameters
{
  // set the initial omegas distribution
  vector<double> omega_initials, omega_frequencies_initials;
  omega_initials.push_back(0.5); omega_initials.push_back(1); omega_initials.push_back(2);
  omega_frequencies_initials.push_back(0.333333); omega_frequencies_initials.push_back(0.333333); omega_frequencies_initials.push_back(0.333334);

  auto psdd = make_unique<SimpleDiscreteDistribution>(omega_initials, omega_frequencies_initials);

  map<string, unique_ptr<DiscreteDistributionInterface>> mpdd;
  mpdd["omega"] = std::move(psdd);

  // build the submodel as a basic Yang Nielsen model (with kappa instead of 5 GTR nucleotide substituion rate parameters)
  auto yn98 = make_unique<YN98>(gc, std::move(codonFreqs));

  // initialize the site model with the initial omegas distribution
  mixedModelPtr_ = make_unique<MixtureOfASubstitutionModel>(gc->getSourceAlphabet(), std::move(yn98), mpdd);
  mixedSubModelPtr_ = dynamic_cast<const MixtureOfASubstitutionModel*>(&mixedModel());

  vector<int> supportedChars = mixedModelPtr_->getAlphabetStates();

  // mapping the parameters
  ParameterList pl = mixedModelPtr_->getParameters();
  for (size_t i = 0; i < pl.size(); ++i)
  {
    lParPmodel_.addParameter(Parameter(pl[i])); // add the parameter to the biblio wrapper instance - see Laurent's response in https://groups.google.com/forum/#!searchin/biopp-help-forum/likelihood$20ratio$20test|sort:date/biopp-help-forum/lH8MYit_Mr8/2CBND79B11YJ
  }

  // v consists of 9 shared theta parameters, that are used for the F3X4 estimation of codon frequencies
  vector<std::string> v = dynamic_cast<const YN98&>(mixedModelPtr_->nModel(0)).frequencySet().getParameters().getParameterNames();

  for (auto& vi : v)
  {
    mapParNamesFromPmodel_[vi] = vi.substr(5);
  }

  // map the parameters of RELAX to the parameters of the sub-models
  mapParNamesFromPmodel_["YN98.kappa"] = "kappa";
  mapParNamesFromPmodel_["YN98.omega_Simple.V1"] = "p";            // omega0=p*omega1 (p is re-parameterization of omega0)
  mapParNamesFromPmodel_["YN98.omega_Simple.V2"] = "omega1";
  mapParNamesFromPmodel_["YN98.omega_Simple.theta1"] = "theta1";  // frequency of omega1 (denoted in YNGP_M2's documentation as p0)
  mapParNamesFromPmodel_["YN98.omega_Simple.V3"] = "omega2";
  mapParNamesFromPmodel_["YN98.omega_Simple.theta2"] = "theta2";  // theta2 = (p1/(p1+p2))
  /* codon frequencies parameterization using F3X4: for each _Full.theta, corresponding to a a codon position over {0,1,2}:
     getFreq_(0) = theta1 * (1. - theta);
     getFreq_(1) = (1 - theta2) * theta;
     getFreq_(2) = theta2 * theta;
     getFreq_(3) = (1 - theta1) * (1. - theta); */

  string st;
  for (auto& it : mapParNamesFromPmodel_)
  {
    st = mixedModelPtr_->getParameterNameWithoutNamespace(it.first);
    if (it.second.substr(0, 5) != "omega" && it.second.substr(0, 5) != "p")
    {
      addParameter_(new Parameter("RELAX." + it.second, mixedModelPtr_->getParameterValue(st),
            mixedModelPtr_->parameter(st).hasConstraint() ? std::shared_ptr<ConstraintInterface>(mixedModelPtr_->parameter(st).getConstraint()->clone()) : 0));
    }
  }

  /* set the below parameters that are used for parameterizing the omega parameters of the sumodels of type YN98 as autoparameters to supress exceptions when constraints of the YN98 omega parameters are exceeded
     YN98_0.omega = (RELAX.p * RELAX.omega1) ^ RELAX.k
     YN98_1.omega = RELAX.omega1 ^ RELAX.k
     YN98_2.omega = RELAX.omega2 ^ RELAX.k */
  // reparameterization of omega0: RELAX.omega0 = RELAX.p*RELAX.omega1
  addParameter_(new Parameter("RELAX.p", 0.5, std::make_shared<IntervalConstraint>(0.01, 1, true, true)));

  addParameter_(new Parameter("RELAX.omega1", 1, std::make_shared<IntervalConstraint>(0.1, 1, true, true)));

  // the upper bound of omega3 in its submodel is 999, so I must restrict upperBound(RELAX.omega2)^upperBound(RELAX.k)<=999 -> set maximal omega to 5
  addParameter_(new Parameter("RELAX.omega2", 2, std::make_shared<IntervalConstraint>(1, 999, true, true)));

  // add a selection intensity parameter k, which is 1 in the null case
  addParameter_(new Parameter("RELAX.k", 1, std::make_shared<IntervalConstraint>(0, 10, false, true))); // selection intensity parameter for purifying and neutral selection parameters

  // look for synonymous codons
  // assumes that the states number follow the map in the genetic code and thus:
  // synfrom_ = index of source codon
  // synto_ = index of destination codon
  for (synfrom_ = 1; synfrom_ < supportedChars.size(); ++synfrom_)
  {
    for (synto_ = 0; synto_ < synfrom_; ++synto_)
    {
      if (gc->areSynonymous(supportedChars[synfrom_], supportedChars[synto_])
          && (mixedSubModelPtr_->subNModel(0).Qij(synfrom_, synto_) != 0)
          && (mixedSubModelPtr_->subNModel(1).Qij(synfrom_, synto_) != 0))
        break;
    }
    if (synto_ < synfrom_)
      break;
  }

  if (synto_ == supportedChars.size())
    throw Exception("Impossible to find synonymous codons");

  // update the 3 rate matrices of the model (strict BG or strict FG)
  computeFrequencies(false);
  updateMatrices_();
}


void RELAX::updateMatrices_()
{
  // update the values of the sub-model parameters, that are used in the 3 rate matrices
  for (unsigned int i = 0; i < lParPmodel_.size(); ++i)
  {
    // first update the values of the non omega patrameters
    const string& np = lParPmodel_[i].getName();
    if (np.size() > 19 && np[18] == 'V')
    {
      double k = getParameterValue("k"); // get the value of k
      int ind = -1;
      if (np.size() > 19)
      {
        ind = TextTools::to<int>(np.substr(19)) - 1; // ind corresponds to the index of the omega that belongs to submodel ind+1
      }
      // change ind not to be -1 to allow names omega0, omega1, omega2
      double omega;
      if (ind == 0)
      {                         // handle omega0 differently due to reparameterization via RELAX.p
        omega = getParameterValue("p") * getParameterValue("omega1");
        omega = pow(omega, k);
        if (omega < 0.002)
        {
          omega = 0.002;
        }
      }
      else if (ind == 1)
      {
        omega = getParameterValue("omega1");
        omega = pow (omega, k);
        if (omega <= 0.002)
        {
          omega = 0.002;
        }
      }
      else
      {
        omega = getParameterValue("omega2");
        omega = pow (omega, k);
        if (omega > 999)
        {
          omega = 999;
        }
      }
      lParPmodel_[i].setValue(omega);
    }
    else
    {
      lParPmodel_[i].setValue(parameter(getParameterNameWithoutNamespace(mapParNamesFromPmodel_[np])).getValue());
    }
  }


  mixedModelPtr_->matchParametersValues(lParPmodel_);

  // normalize the synonymous substitution rate in all the Q matrices of the 3 submodels to be the same
  Vdouble vd;

  for (unsigned int i = 0; i < mixedModelPtr_->getNumberOfModels(); ++i)
  {
    vd.push_back(1 / mixedSubModelPtr_->subNModel(i).Qij(synfrom_, synto_));
  }

  mixedModelPtr_->setVRates(vd);
}
