// SPDX-FileCopyrightText: The Bio++ Development Group
//
// SPDX-License-Identifier: CECILL-2.1

#include <Bpp/Numeric/Random/RandomTools.h>
#include <Bpp/Text/TextTools.h>

#include "MutationProcess.h"

using namespace bpp;
using namespace std;

/******************************************************************************/

size_t AbstractMutationProcess::mutate(size_t state) const
{
  double alea = RandomTools::giveRandomNumberBetweenZeroAndEntry(1.0);
  for (size_t j = 0; j < size_; j++)
  {
    if (alea < repartition_[state][j])
      return j;
  }
  throw Exception("AbstractMutationProcess::mutate. Repartition function is incomplete for state " + TextTools::toString(state));
}

/******************************************************************************/

size_t AbstractMutationProcess::mutate(size_t state, unsigned int n) const
{
  size_t s = state;
  for (unsigned int k = 0; k < n; k++)
  {
    double alea = RandomTools::giveRandomNumberBetweenZeroAndEntry(1.0);
    for (size_t j = 1; j < size_ + 1; j++)
    {
      if (alea < repartition_[s][j])
      {
        s = j;
        break;
      }
    }
  }
  return s;
}

/******************************************************************************/

double AbstractMutationProcess::getTimeBeforeNextMutationEvent(size_t state) const
{
  return RandomTools::randExponential(-1. / model_->Qij(state, state));
}

/******************************************************************************/

size_t AbstractMutationProcess::evolve(size_t initialState, double time) const
{
  double t = 0;
  size_t currentState = initialState;
  t += getTimeBeforeNextMutationEvent(currentState);
  while (t < time)
  {
    currentState = mutate(currentState);
    t += getTimeBeforeNextMutationEvent(currentState);
  }
  return currentState;
}

/******************************************************************************/

MutationPath AbstractMutationProcess::detailedEvolve(size_t initialState, double time) const
{
  MutationPath mp(model_->getAlphabet(), initialState, time);
  double t = 0;
  size_t currentState = initialState;

  t += getTimeBeforeNextMutationEvent(currentState);
  while (t < time)
  {
    currentState = mutate(currentState);
    mp.addEvent(currentState, t);
    t += getTimeBeforeNextMutationEvent(currentState);
  }

  return mp;
}

/******************************************************************************/

MutationPath AbstractMutationProcess::detailedEvolve(size_t initialState, size_t finalState, double time) const
{
  size_t maxIterNum = 1000; // max nb of tries
  MutationPath mp(model_->getAlphabet(), initialState, time);

  for (size_t i = 0; i < maxIterNum; ++i)
  {
    mp.clear();

    double t = 0;

    // if the father's state is not the same as the son's state -> use the correction corresponding to equation (11) in the paper
    if (initialState != finalState)
    {   // sample timeTillChange conditional on it being smaller than time
      double u = RandomTools::giveRandomNumberBetweenZeroAndEntry(1.0);
      double waitingTimeParam = model_->Qij(initialState, initialState); // get the parameter for the exponential distribution to draw the waiting time
      double tmp = u * (1.0 - exp(time * waitingTimeParam));
      t =  log(1.0 - tmp) / waitingTimeParam;
    }
    else
    {
      t = getTimeBeforeNextMutationEvent(initialState); // draw the time until a transition from exponential distribution with the rate of leaving fatherState
    }

    size_t currentState = initialState;
    while (t < time)  // a jump occured but not passed the whole time
    {
      currentState = mutate(currentState);
      mp.addEvent(currentState, t);      // add the current state and time to branch history
      t += getTimeBeforeNextMutationEvent(currentState);        // draw the time until a transition from exponential distribution with the rate of leaving currentState from initial state curState based on the relative tranistion rates distribution
    }
    //   // the last jump passed the length of the branch -> finish the simulation and check if it's sucessfull (i.e, mapping is finished at the son's state)
    if (currentState != finalState) // if the simulation failed, try again
    {
      continue;
    }
    else
      return mp;
  }

  // Emergency case when none simul reached finalState
  mp.addEvent(finalState, time);
  return mp;
}


/******************************************************************************/

SimpleMutationProcess::SimpleMutationProcess(
    shared_ptr<const SubstitutionModelInterface> model) :
  AbstractMutationProcess(model)
{
  size_ = model->getNumberOfStates();
  repartition_ = VVdouble(size_);
  // Each element contains the probabilities concerning each character in the alphabet.

  // We will now initiate each of these probability vector.
  RowMatrix<double> Q = model->generator();
  for (size_t i = 0; i < size_; i++)
  {
    repartition_[i] = Vdouble(size_, 0);
    if (abs(Q(i, i)) > NumConstants::TINY())
    {
      double cum = 0;
      double sum_Q = 0;
      for (size_t j = 0; j < size_; j++)
      {
        if (j != i)
          sum_Q += Q(i, j);
      }

      for (size_t j = 0; j < size_; j++)
      {
        if (j != i)
        {
          cum += model->Qij(i, j) / sum_Q;
          repartition_[i][j] = cum;
        }
        else
          repartition_[i][j] = -1;
        // Forbiden value: does not correspond to a change.
      }
    }
  }

  // Note that I use cumulative probabilities in repartition_ (hence the name).
  // These cumulative probabilities are useful for the 'mutate(...)' function.
}

SimpleMutationProcess::~SimpleMutationProcess() {}

/******************************************************************************/

size_t SimpleMutationProcess::evolve(size_t initialState, double time) const
{
  // Compute all cumulative pijt:
  Vdouble pijt(size_);
  pijt[0] = model_->Pij_t(initialState, 0, time);
  for (size_t i = 1; i < size_; i++)
  {
    pijt[i] = pijt[i - 1] + model_->Pij_t(initialState, i, time);
  }
  double rand = RandomTools::giveRandomNumberBetweenZeroAndEntry(1);
  for (size_t i = 0; i < size_; i++)
  {
    if (rand < pijt[i])
      return i;
  }
  throw Exception("SimpleSimulationProcess::evolve(intialState, time): error all pijt do not sum to one (total sum = " + TextTools::toString(pijt[size_ - 1]) + ").");
}

/******************************************************************************/

SelfMutationProcess::SelfMutationProcess(size_t alphabetSize) :
  AbstractMutationProcess(0)
{
  size_ = alphabetSize;
  repartition_ = VVdouble(size_);
  // Each element contains the probabilities concerning each character in the alphabet.

  // We will now initiate each of these probability vector.
  for (size_t i = 0; i < size_; i++)
  {
    repartition_[i] = Vdouble(size_);
    for (size_t j = 0; j < size_; j++)
    {
      repartition_[i][j] = static_cast<double>(j + 1) / static_cast<double>(size_);
    }
  }
  // Note that I use cumulative probabilities in repartition_ (hence the name).
  // These cumulative probabilities are useful for the 'mutate(...)' function.
}

SelfMutationProcess::~SelfMutationProcess() {}

/******************************************************************************/
