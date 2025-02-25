// SPDX-FileCopyrightText: The Bio++ Development Group
//
// SPDX-License-Identifier: CECILL-2.1

#include "../StateMap.h"
#include "CodonFrequencySet.h"
#include "NucleotideFrequencySet.h"

// From bpp-core:
#include <Bpp/Numeric/Prob/Simplex.h>

using namespace bpp;
using namespace std;

// ////////////////////////////
// FullCodonFrequencySet

FullCodonFrequencySet::FullCodonFrequencySet(
    shared_ptr<const GeneticCode> gCode,
    bool allowNullFreqs,
    unsigned short method,
    const string& name) :
  AbstractFrequencySet(
      make_shared<CanonicalStateMap>(gCode->getSourceAlphabet(), false),
      "Full.",
      name),
  pgc_(gCode),
  sFreq_(gCode->sourceAlphabet().getSize()  - gCode->getNumberOfStopCodons(), method, allowNullFreqs, "Full.")
{
  vector<double> vd;
  double r = 1. / static_cast<double>(sFreq_.dimension());

  for (size_t i = 0; i < sFreq_.dimension(); ++i)
  {
    vd.push_back(r);
  }

  sFreq_.setFrequencies(vd);
  addParameters_(sFreq_.getParameters());
  updateFreq_();
}

FullCodonFrequencySet::FullCodonFrequencySet(
    shared_ptr<const GeneticCode> gCode,
    const vector<double>& initFreqs,
    bool allowNullFreqs,
    unsigned short method,
    const string& name) :
  AbstractFrequencySet(
      make_shared<CanonicalStateMap>(gCode->getSourceAlphabet(), false),
      "Full.",
      name),
  pgc_(gCode),
  sFreq_(gCode->sourceAlphabet().getSize() - gCode->getNumberOfStopCodons(), method, allowNullFreqs, "Full.")
{
  if (initFreqs.size() != getCodonAlphabet()->getSize())
    throw Exception("FullCodonFrequencySet(constructor). There must be " + TextTools::toString(gCode->getSourceAlphabet()->getSize()) + " frequencies.");

  double sum = 0;
  for (size_t i = 0; i < getCodonAlphabet()->getSize(); i++)
  {
    if (!pgc_->isStop(static_cast<int>(i)))
      sum += initFreqs[i];
  }

  vector<double> vd;
  for (size_t i = 0; i < getCodonAlphabet()->getSize(); i++)
  {
    if (!gCode->isStop(static_cast<int>(i)))
      vd.push_back(initFreqs[i] / sum);
  }

  sFreq_.setFrequencies(vd);
  addParameters_(sFreq_.getParameters());
  updateFreq_();
}

FullCodonFrequencySet::FullCodonFrequencySet(const FullCodonFrequencySet& fcfs) :
  AbstractFrequencySet(fcfs),
  pgc_(fcfs.pgc_),
  sFreq_(fcfs.sFreq_)
{}

FullCodonFrequencySet& FullCodonFrequencySet::operator=(const FullCodonFrequencySet& fcfs)
{
  AbstractFrequencySet::operator=(fcfs);
  pgc_ = fcfs.pgc_;
  sFreq_ = fcfs.sFreq_;

  return *this;
}

void FullCodonFrequencySet::setNamespace(const std::string& nameSpace)
{
  sFreq_.setNamespace(nameSpace);
  AbstractFrequencySet::setNamespace(nameSpace);
}

void FullCodonFrequencySet::setFrequencies(const vector<double>& frequencies)
{
  if (frequencies.size() != getCodonAlphabet()->getSize())
    throw DimensionException("FullFrequencySet::setFrequencies", frequencies.size(), getAlphabet()->getSize());

  double sum = 0;
  for (size_t i = 0; i < getCodonAlphabet()->getSize(); i++)
  {
    if (!pgc_->isStop(static_cast<int>(i)))
      sum += frequencies[i];
  }

  vector<double> vd;
  for (size_t i = 0; i < getCodonAlphabet()->getSize(); i++)
  {
    if (!pgc_->isStop(static_cast<int>(i)))
      vd.push_back(frequencies[i] / sum);
  }

  sFreq_.setFrequencies(vd);
  setParametersValues(sFreq_.getParameters());
  updateFreq_();
}

void FullCodonFrequencySet::fireParameterChanged(const ParameterList& parameters)
{
  sFreq_.matchParametersValues(parameters);
  updateFreq_();
}

void FullCodonFrequencySet::updateFreq_()
{
  size_t nbstop = 0;

  for (size_t j = 0; j < getCodonAlphabet()->getSize(); j++)
  {
    if (pgc_->isStop(static_cast<int>(j)))
    {
      getFreq_(j) = 0;
      nbstop++;
    }
    else
      getFreq_(j) = sFreq_.prob(j - nbstop);
  }
}


// ////////////////////////////
// FullPerAACodonFrequencySet

FullPerAACodonFrequencySet::FullPerAACodonFrequencySet(
    shared_ptr<const GeneticCode> gencode,
    unique_ptr<ProteinFrequencySetInterface> ppfs,
    unsigned short method) :
  AbstractFrequencySet(
      make_shared<CanonicalStateMap>(gencode->getSourceAlphabet(), false),
      "FullPerAA.",
      "FullPerAA"),
  pgc_(gencode),
  ppfs_(std::move(ppfs)),
  vS_()
{
  auto& ppa = pgc_->proteicAlphabet();
  auto& aaStates = ppfs_->stateMap();
  for (size_t i = 0; i < aaStates.getNumberOfModelStates(); ++i)
  {
    int aa = aaStates.getAlphabetStateAsInt(i);
    vector<int> vc = pgc_->getSynonymous(aa);
    vS_.push_back(Simplex(vc.size(), method, 0, ""));

    Simplex& si = vS_[i];

    si.setNamespace("FullPerAA." + ppa.getAbbr(aa) + "_");
    addParameters_(si.getParameters());
  }

  ppfs_->setNamespace("FullPerAA." + ppfs_->getName() + ".");
  addParameters_(ppfs_->getParameters());

  updateFrequencies_();
}

FullPerAACodonFrequencySet::FullPerAACodonFrequencySet(
    shared_ptr<const GeneticCode> gencode,
    unsigned short method) :
  AbstractFrequencySet(
      make_shared<CanonicalStateMap>(gencode->getSourceAlphabet(), false),
      "FullPerAA.",
      "FullPerAA"),
  pgc_(gencode),
  ppfs_(new FixedProteinFrequencySet(gencode->getProteicAlphabet(), "FullPerAA.")),
  vS_()
{
  auto ppa = pgc_->getProteicAlphabet();
  auto& aaStates = ppfs_->stateMap();
  for (size_t i = 0; i < aaStates.getNumberOfModelStates(); ++i)
  {
    int aa = aaStates.getAlphabetStateAsInt(i);
    vector<int> vc = pgc_->getSynonymous(aa);
    vS_.push_back(Simplex(vc.size(), method, 0, ""));

    Simplex& si = vS_[i];
    si.setNamespace("FullPerAA." + ppa->getAbbr(aa) + "_");
    addParameters_(si.getParameters());
  }

  updateFrequencies_();
}

FullPerAACodonFrequencySet::FullPerAACodonFrequencySet(const FullPerAACodonFrequencySet& ffs) :
  AbstractFrequencySet(ffs),
  pgc_(ffs.pgc_),
  ppfs_(ffs.ppfs_->clone()),
  vS_(ffs.vS_)
{
  updateFrequencies_();
}

FullPerAACodonFrequencySet& FullPerAACodonFrequencySet::operator=(const FullPerAACodonFrequencySet& ffs)
{
  AbstractFrequencySet::operator=(ffs);
  pgc_ = ffs.pgc_;
  ppfs_.reset(ffs.ppfs_->clone());
  vS_ = ffs.vS_;

  return *this;
}

void FullPerAACodonFrequencySet::fireParameterChanged(const ParameterList& parameters)
{
  if (dynamic_cast<AbstractFrequencySet*>(ppfs_.get()))
    (dynamic_cast<AbstractFrequencySet*>(ppfs_.get()))->matchParametersValues(parameters);
  for (size_t i = 0; i < vS_.size(); i++)
  {
    vS_[i].matchParametersValues(parameters);
  }
  updateFrequencies_();
}

void FullPerAACodonFrequencySet::updateFrequencies_()
{
  auto& aaStates = ppfs_->stateMap();
  for (size_t i = 0; i < aaStates.getNumberOfModelStates(); ++i)
  {
    int aa = aaStates.getAlphabetStateAsInt(i);
    std::vector<int> vc = pgc_->getSynonymous(aa);
    for (size_t j = 0; j < vc.size(); j++)
    {
      // NB: only one alphabet state per model state here, as it is a CodonFreqSet.
      getFreq_(stateMap().getModelStates(vc[j])[0]) =
          static_cast<double>(vc.size()) * ppfs_->getFrequencies()[i] * vS_[i].prob(j);
    }
  }
  normalize();
}

void FullPerAACodonFrequencySet::setFrequencies(const vector<double>& frequencies)
{
  if (frequencies.size() != getCodonAlphabet()->getSize())
    throw DimensionException("FullPerAAFrequencySet::setFrequencies", frequencies.size(), getCodonAlphabet()->getSize());

  double bigS = 0;
  Vdouble vaa;

  auto& aaStates = ppfs_->stateMap();
  for (size_t i = 0; i < aaStates.getNumberOfModelStates(); ++i)
  {
    int aa = aaStates.getAlphabetStateAsInt(i);
    std::vector<int> vc = pgc_->getSynonymous(aa);
    Vdouble vp;
    double s = 0;

    for (size_t j = 0; j < vc.size(); j++)
    {
      size_t index = pgc_->getSourceAlphabet()->getStateIndex(vc[j]);
      vp.push_back(frequencies[index - 1]);
      s += frequencies[index - 1];
    }

    vp /= s;
    vS_[i].setFrequencies(vp);

    matchParametersValues(vS_[i].getParameters());

    bigS += s / static_cast<double>(vc.size());
    vaa.push_back(s / static_cast<double>(vc.size()));
  }

  vaa /= bigS; // to avoid counting of stop codons
  ppfs_->setFrequencies(vaa);
  matchParametersValues(ppfs_->getParameters());
  updateFrequencies_();
}

void FullPerAACodonFrequencySet::setNamespace(const std::string& prefix)
{
  auto ppa = pgc_->getProteicAlphabet();

  AbstractFrequencySet::setNamespace(prefix);
  ppfs_->setNamespace(prefix + ppfs_->getName() + ".");
  for (size_t i = 0; i < vS_.size(); ++i)
  {
    vS_[i].setNamespace(prefix + ppa->getAbbr(static_cast<int>(i)) + "_");
  }
}


// ///////////////////////////////////////////
// / FixedCodonFrequencySet

FixedCodonFrequencySet::FixedCodonFrequencySet(
    shared_ptr<const GeneticCode> gCode,
    const vector<double>& initFreqs,
    const string& name) :
  AbstractFrequencySet(
      make_shared<CanonicalStateMap>(gCode->getSourceAlphabet(), false),
      "Fixed.",
      name),
  pgc_(gCode)
{
  setFrequencies(initFreqs);
}

FixedCodonFrequencySet::FixedCodonFrequencySet(
    shared_ptr<const GeneticCode> gCode,
    const string& name) :
  AbstractFrequencySet(
      make_shared<CanonicalStateMap>(gCode->getSourceAlphabet(), false),
      "Fixed.",
      name),
  pgc_(gCode)
{
  size_t size = gCode->sourceAlphabet().getSize() - gCode->getNumberOfStopCodons();

  for (size_t i = 0; i < gCode->sourceAlphabet().getSize(); i++)
  {
    getFreq_(i) = (gCode->isStop(static_cast<int>(i))) ? 0 : 1. / static_cast<double>(size);
  }
}

void FixedCodonFrequencySet::setFrequencies(const vector<double>& frequencies)
{
  auto ca = getCodonAlphabet();
  if (frequencies.size() != ca->getSize())
    throw DimensionException("FixedFrequencySet::setFrequencies", frequencies.size(), ca->getSize());
  double sum = 0.0;

  for (size_t i = 0; i < frequencies.size(); ++i)
  {
    if (!(pgc_->isStop(static_cast<int>(i))))
      sum += frequencies[i];
  }

  for (size_t i = 0; i < ca->getSize(); ++i)
  {
    getFreq_(i) = (pgc_->isStop(static_cast<int>(i))) ? 0 : frequencies[i] / sum;
  }
}


// ///////////////////////////////////////////
// / UserCodonFrequencySet

UserCodonFrequencySet::UserCodonFrequencySet(
    shared_ptr<const GeneticCode> gCode,
    const std::string& path,
    size_t nCol) :
  UserFrequencySet(
      make_shared<CanonicalStateMap>(gCode->getSourceAlphabet(), false),
      path,
      nCol),
  pgc_(gCode)
{}

void UserCodonFrequencySet::setFrequencies(const vector<double>& frequencies)
{
  auto ca = getCodonAlphabet();
  if (frequencies.size() != ca->getSize())
    throw DimensionException("UserFrequencySet::setFrequencies", frequencies.size(), ca->getSize());
  double sum = 0.0;

  for (size_t i = 0; i < frequencies.size(); ++i)
  {
    if (!(pgc_->isStop(static_cast<int>(i))))
      sum += frequencies[i];
  }

  for (size_t i = 0; i < ca->getSize(); ++i)
  {
    getFreq_(i) = (pgc_->isStop(static_cast<int>(i))) ? 0 : frequencies[i] / sum;
  }
}


// ///////////////////////////////////////////////////////////////////
// // CodonFromIndependentFrequencySet


CodonFromIndependentFrequencySet::CodonFromIndependentFrequencySet(
    shared_ptr<const GeneticCode> gCode,
    vector<unique_ptr<FrequencySetInterface>>& freqvector,
    const string& name,
    const string& mgmtStopCodon) :
  WordFromIndependentFrequencySet(
      gCode->getCodonAlphabet(),
      freqvector,
      "",
      name),
  mStopNeigh_(),
  mgmtStopCodon_(2),
  pgc_(gCode)
{
  if (mgmtStopCodon == "uniform")
    mgmtStopCodon_ = 0;
  else if (mgmtStopCodon == "linear")
    mgmtStopCodon_ = 1;

  // fill the map of the stop codons

  vector<int> vspcod = gCode->getStopCodonsAsInt();
  for (size_t ispcod = 0; ispcod < vspcod.size(); ispcod++)
  {
    size_t pow = 1;
    int nspcod = vspcod[ispcod];
    for (size_t ph = 0; ph < 3; ph++)
    {
      size_t nspcod0 = static_cast<size_t>(nspcod) - pow * static_cast<size_t>(getCodonAlphabet()->getNPosition(nspcod, 2 - ph));
      for (size_t dec = 0; dec < 4; dec++)
      {
        size_t vois = nspcod0 + pow * dec;
        if (!pgc_->isStop(static_cast<int>(vois)))
          mStopNeigh_[nspcod].push_back(static_cast<int>(vois));
      }
      pow *= 4;
    }
  }
  updateFrequencies();
}

shared_ptr<const CodonAlphabet> CodonFromIndependentFrequencySet::getCodonAlphabet() const
{
  return dynamic_pointer_cast<const CodonAlphabet>(WordFromIndependentFrequencySet::getAlphabet());
}

CodonFromIndependentFrequencySet::CodonFromIndependentFrequencySet(const CodonFromIndependentFrequencySet& iwfs) :
  WordFromIndependentFrequencySet(iwfs),
  mStopNeigh_(iwfs.mStopNeigh_),
  mgmtStopCodon_(iwfs.mgmtStopCodon_),
  pgc_(iwfs.pgc_)
{
  updateFrequencies();
}

CodonFromIndependentFrequencySet& CodonFromIndependentFrequencySet::operator=(const CodonFromIndependentFrequencySet& iwfs)
{
  WordFromIndependentFrequencySet::operator=(iwfs);
  mStopNeigh_ = iwfs.mStopNeigh_;
  mgmtStopCodon_ = iwfs.mgmtStopCodon_;
  pgc_ = iwfs.pgc_;
  return *this;
}

void CodonFromIndependentFrequencySet::updateFrequencies()
{
  WordFromIndependentFrequencySet::updateFrequencies();

  size_t s = getCodonAlphabet()->getSize();

  if (mgmtStopCodon_ != 0)
  {
    // The frequencies of the stop codons are distributed to all
    // neighbour non-stop codons
    double f[64];
    for (size_t i = 0; i < s; i++)
    {
      f[i] = 0;
    }

    std::map<int, Vint>::iterator mStopNeigh_it(mStopNeigh_.begin());
    while (mStopNeigh_it != mStopNeigh_.end())
    {
      int stNb = mStopNeigh_it->first;
      Vint vneigh = mStopNeigh_it->second;
      double sneifreq = 0;
      for (size_t vn = 0; vn < vneigh.size(); vn++)
      {
        sneifreq += pow(getFreq_(static_cast<size_t>(vneigh[vn])), mgmtStopCodon_);
      }
      double x = getFreq_(static_cast<size_t>(stNb)) / sneifreq;
      for (size_t vn = 0; vn < vneigh.size(); vn++)
      {
        f[vneigh[vn]] += pow(getFreq_(static_cast<size_t>(vneigh[vn])), mgmtStopCodon_) * x;
      }
      getFreq_(static_cast<size_t>(stNb)) = 0;
      mStopNeigh_it++;
    }

    for (size_t i = 0; i < s; i++)
    {
      getFreq_(i) += f[i];
    }
  }
  else
  {
    double sum = 0.;
    for (size_t i = 0; i < s; i++)
    {
      if (!pgc_->isStop(static_cast<int>(i)))
        sum += getFreq_(i);
    }

    for (size_t i = 0; i < s; i++)
    {
      if (pgc_->isStop(static_cast<int>(i)))
        getFreq_(i) = 0;
      else
        getFreq_(i) /= sum;
    }
  }
}

// ///////////////////////////////////////////////////////////////////
// // CodonFromUniqueFrequencySet


CodonFromUniqueFrequencySet::CodonFromUniqueFrequencySet(
    shared_ptr<const GeneticCode> gCode,
    unique_ptr<FrequencySetInterface> pfreq,
    const string& name,
    const string& mgmtStopCodon) :
  WordFromUniqueFrequencySet(gCode->getCodonAlphabet(), std::move(pfreq), "", name),
  mStopNeigh_(),
  mgmtStopCodon_(2),
  pgc_(gCode)
{
  if (mgmtStopCodon == "uniform")
    mgmtStopCodon_ = 0;
  else if (mgmtStopCodon == "linear")
    mgmtStopCodon_ = 1;

  // fill the map of the stop codons

  vector<int> vspcod = gCode->getStopCodonsAsInt();
  for (size_t ispcod = 0; ispcod < vspcod.size(); ispcod++)
  {
    size_t pow = 1;
    int nspcod = vspcod[ispcod];
    for (int ph = 0; ph < 3; ph++)
    {
      size_t nspcod0 = static_cast<size_t>(nspcod) - pow * static_cast<size_t>(getCodonAlphabet()->getNPosition(nspcod, static_cast<unsigned int>(2 - ph)));
      for (size_t dec = 0; dec < 4; dec++)
      {
        size_t vois = nspcod0 + pow * dec;
        if (!pgc_->isStop(static_cast<int>(vois)))
          mStopNeigh_[nspcod].push_back(static_cast<int>(vois));
      }
      pow *= 4;
    }
  }

  updateFrequencies();
}

shared_ptr<const CodonAlphabet> CodonFromUniqueFrequencySet::getCodonAlphabet() const
{
  return dynamic_pointer_cast<const CodonAlphabet>(WordFromUniqueFrequencySet::getWordAlphabet());
}


CodonFromUniqueFrequencySet::CodonFromUniqueFrequencySet(const CodonFromUniqueFrequencySet& iwfs) :
  WordFromUniqueFrequencySet(iwfs),
  mStopNeigh_(iwfs.mStopNeigh_),
  mgmtStopCodon_(iwfs.mgmtStopCodon_),
  pgc_(iwfs.pgc_)
{
  updateFrequencies();
}

CodonFromUniqueFrequencySet& CodonFromUniqueFrequencySet::operator=(const CodonFromUniqueFrequencySet& iwfs)
{
  WordFromUniqueFrequencySet::operator=(iwfs);
  mStopNeigh_ = iwfs.mStopNeigh_;
  mgmtStopCodon_ = iwfs.mgmtStopCodon_;
  pgc_ = iwfs.pgc_;
  return *this;
}

void CodonFromUniqueFrequencySet::updateFrequencies()
{
  WordFromUniqueFrequencySet::updateFrequencies();

  size_t s = getCodonAlphabet()->getSize();

  if (mgmtStopCodon_ != 0)
  {
    // The frequencies of the stop codons are distributed to all
    // neighbour non-stop codons
    double f[64] = {0};

    for (const auto& mStopNeigh_it : mStopNeigh_)
    {
      int stNb = mStopNeigh_it.first;
      Vint vneigh = mStopNeigh_it.second;
      double sneifreq = 0;
      for (size_t vn = 0; vn < vneigh.size(); vn++)
      {
        sneifreq += pow(getFreq_(static_cast<size_t>(vneigh[vn])), mgmtStopCodon_);
      }
      double x = getFreq_(static_cast<size_t>(stNb)) / sneifreq;
      for (size_t vn = 0; vn < vneigh.size(); vn++)
      {
        f[vneigh[vn]] += pow(getFreq_(static_cast<size_t>(vneigh[vn])), mgmtStopCodon_) * x;
      }
      getFreq_(static_cast<size_t>(stNb)) = 0;
    }

    for (size_t i = 0; i < s; i++)
    {
      getFreq_(i) += f[i];
    }
  }
  else
  {
    double sum = 0.;
    for (size_t i = 0; i < s; i++)
    {
      if (!pgc_->isStop(static_cast<int>(i)))
        sum += getFreq_(i);
    }

    for (unsigned int i = 0; i < s; i++)
    {
      if (pgc_->isStop(static_cast<int>(i)))
        getFreq_(i) = 0;
      else
        getFreq_(i) /= sum;
    }
  }
}

/*********************************************************************/

unique_ptr<CodonFrequencySetInterface> CodonFrequencySetInterface::getFrequencySetForCodons(
    short option,
    shared_ptr<const GeneticCode> gCode,
    const string& mgmtStopCodon,
    unsigned short method)
{
  unique_ptr<CodonFrequencySetInterface> codonFreqs;

  if (option == F0)
  {
    codonFreqs.reset(new FixedCodonFrequencySet(gCode, "F0"));
  }
  else if (option == F1X4)
  {
    codonFreqs.reset(new CodonFromUniqueFrequencySet(gCode, make_unique<FullNucleotideFrequencySet>(gCode->codonAlphabet().getNucleicAlphabet()), "F1X4", mgmtStopCodon));
  }
  else if (option == F3X4)
  {
    vector<unique_ptr<FrequencySetInterface>> v_AFS(3);
    v_AFS[0] = make_unique<FullNucleotideFrequencySet>(gCode->codonAlphabet().getNucleicAlphabet());
    v_AFS[1] = make_unique<FullNucleotideFrequencySet>(gCode->codonAlphabet().getNucleicAlphabet());
    v_AFS[2] = make_unique<FullNucleotideFrequencySet>(gCode->codonAlphabet().getNucleicAlphabet());
    codonFreqs = make_unique<CodonFromIndependentFrequencySet>(gCode, v_AFS, "F3X4", mgmtStopCodon);
  }
  else if (option == F61)
    codonFreqs.reset(new FullCodonFrequencySet(gCode, false, method, "F61"));
  else
    throw Exception("FrequencySet::getFrequencySetForCodons(). Unvalid codon frequency set argument.");

  return codonFreqs;
}

/******************************************************************************/

const short CodonFrequencySetInterface::F0   = 0;
const short CodonFrequencySetInterface::F1X4 = 1;
const short CodonFrequencySetInterface::F3X4 = 2;
const short CodonFrequencySetInterface::F61  = 3;

/******************************************************************************/
