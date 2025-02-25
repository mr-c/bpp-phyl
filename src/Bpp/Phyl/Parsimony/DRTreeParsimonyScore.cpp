// SPDX-FileCopyrightText: The Bio++ Development Group
//
// SPDX-License-Identifier: CECILL-2.1

#include <Bpp/App/ApplicationTools.h>
#include <Bpp/Numeric/VectorTools.h>

#include "../PatternTools.h"
#include "../Tree/TreeTemplateTools.h" // Needed for NNIs
#include "DRTreeParsimonyScore.h"

using namespace bpp;
using namespace std;

/******************************************************************************/

DRTreeParsimonyScore::DRTreeParsimonyScore(
    shared_ptr<TreeTemplate<Node>> tree,
    shared_ptr<const SiteContainerInterface> data,
    bool verbose,
    bool includeGaps) :
  AbstractTreeParsimonyScore(tree, data, verbose, includeGaps),
  parsimonyData_(new DRTreeParsimonyData(tree)),
  nbDistinctSites_()
{
  init_(data, verbose);
}

DRTreeParsimonyScore::DRTreeParsimonyScore(
    shared_ptr<TreeTemplate<Node>> tree,
    shared_ptr<const SiteContainerInterface> data,
    shared_ptr<const StateMapInterface> statesMap,
    bool verbose) :
  AbstractTreeParsimonyScore(tree, data, statesMap, verbose),
  parsimonyData_(new DRTreeParsimonyData(tree)),
  nbDistinctSites_()
{
  init_(data, verbose);
}

void DRTreeParsimonyScore::init_(shared_ptr<const SiteContainerInterface> data, bool verbose)
{
  if (verbose)
    ApplicationTools::displayTask("Initializing data structure");
  parsimonyData_->init(data, getStateMap());
  nbDistinctSites_ = parsimonyData_->getNumberOfDistinctSites();
  computeScores();
  if (verbose)
    ApplicationTools::displayTaskDone();
  if (verbose)
    ApplicationTools::displayResult("Number of distinct sites",
        TextTools::toString(nbDistinctSites_));
}

/******************************************************************************/

DRTreeParsimonyScore::DRTreeParsimonyScore(const DRTreeParsimonyScore& tp) :
  AbstractTreeParsimonyScore(tp),
  parsimonyData_(dynamic_cast<DRTreeParsimonyData*>(tp.parsimonyData_->clone())),
  nbDistinctSites_(tp.nbDistinctSites_)
{
  parsimonyData_->setTree(getTreeTemplate());
}

/******************************************************************************/

DRTreeParsimonyScore& DRTreeParsimonyScore::operator=(const DRTreeParsimonyScore& tp)
{
  AbstractTreeParsimonyScore::operator=(tp);
  parsimonyData_.reset(tp.parsimonyData_->clone());
  parsimonyData_->setTree(getTreeTemplate());
  nbDistinctSites_ = tp.nbDistinctSites_;
  return *this;
}

/******************************************************************************/

DRTreeParsimonyScore::~DRTreeParsimonyScore() {}

/******************************************************************************/

void DRTreeParsimonyScore::computeScores()
{
  computeScoresPostorder(treeTemplate().getRootNode());
  computeScoresPreorder(treeTemplate().getRootNode());
  computeScoresForNode(
      parsimonyData_->nodeData(treeTemplate().getRootId()),
      parsimonyData_->getRootBitsets(),
      parsimonyData_->getRootScores());
}

void DRTreeParsimonyScore::computeScoresPostorder(const Node* node)
{
  if (node->isLeaf())
    return;
  DRTreeParsimonyNodeData& pData = parsimonyData_->nodeData(node->getId());
  for (unsigned int k = 0; k < node->getNumberOfSons(); k++)
  {
    const Node* son = node->getSon(k);
    computeScoresPostorder(son);
    vector<Bitset>* bitsets      = &pData.getBitsetsArrayForNeighbor(son->getId());
    vector<unsigned int>* scores = &pData.getScoresArrayForNeighbor(son->getId());
    if (son->isLeaf())
    {
      // son has no NodeData associated, must use LeafData instead
      vector<Bitset>* sonBitsets = &parsimonyData_->leafData(son->getId()).getBitsetsArray();
      for (unsigned int i = 0; i < sonBitsets->size(); i++)
      {
        (*bitsets)[i] = (*sonBitsets)[i];
        (*scores)[i]  = 0;
      }
    }
    else
    {
      computeScoresPostorderForNode(
          parsimonyData_->nodeData(son->getId()),
          *bitsets,
          *scores);
    }
  }
}

void DRTreeParsimonyScore::computeScoresPostorderForNode(const DRTreeParsimonyNodeData& pData, vector<Bitset>& rBitsets, vector<unsigned int>& rScores)
{
  // First initialize the vectors from input:
  const Node* node = pData.getNode();
  const Node* source = node->getFather();
  vector<const Node*> neighbors = node->getNeighbors();
  size_t nbNeighbors = node->degree();
  vector< const vector<Bitset>*> iBitsets;
  vector< const vector<unsigned int>*> iScores;
  for (unsigned int k = 0; k < nbNeighbors; k++)
  {
    const Node* n = neighbors[k];
    if (n != source)
    {
      iBitsets.push_back(&pData.getBitsetsArrayForNeighbor(n->getId()));
      iScores.push_back(&pData.getScoresArrayForNeighbor(n->getId()));
    }
  }
  // Then call the general method on these arrays:
  computeScoresFromArrays(iBitsets, iScores, rBitsets, rScores);
}

void DRTreeParsimonyScore::computeScoresPreorder(const Node* node)
{
  if (node->getNumberOfSons() == 0)
    return;
  DRTreeParsimonyNodeData& pData = parsimonyData_->nodeData(node->getId());
  if (node->hasFather())
  {
    const Node* father = node->getFather();
    vector<Bitset>* bitsets      = &pData.getBitsetsArrayForNeighbor(father->getId());
    vector<unsigned int>* scores = &pData.getScoresArrayForNeighbor(father->getId());
    if (father->isLeaf())
    { // Means that the tree is rooted by a leaf... dunno if we must allow that! Let it be for now.
      // son has no NodeData associated, must use LeafData instead
      vector<Bitset>* sonBitsets = &parsimonyData_->leafData(father->getId()).getBitsetsArray();
      for (unsigned int i = 0; i < sonBitsets->size(); i++)
      {
        (*bitsets)[i] = (*sonBitsets)[i];
        (*scores)[i]  = 0;
      }
    }
    else
    {
      computeScoresPreorderForNode(
          parsimonyData_->nodeData(father->getId()),
          node,
          *bitsets,
          *scores);
    }
  }
  // Recurse call:
  for (unsigned int k = 0; k < node->getNumberOfSons(); k++)
  {
    computeScoresPreorder(node->getSon(k));
  }
}

void DRTreeParsimonyScore::computeScoresPreorderForNode(const DRTreeParsimonyNodeData& pData, const Node* source, std::vector<Bitset>& rBitsets, std::vector<unsigned int>& rScores)
{
  // First initialize the vectors from input:
  const Node* node = pData.getNode();
  vector<const Node*> neighbors = node->getNeighbors();
  size_t nbNeighbors = node->degree();
  vector< const vector<Bitset>*> iBitsets;
  vector< const vector<unsigned int>*> iScores;
  for (unsigned int k = 0; k < nbNeighbors; k++)
  {
    const Node* n = neighbors[k];
    if (n != source)
    {
      iBitsets.push_back(&pData.getBitsetsArrayForNeighbor(n->getId()));
      iScores.push_back(&pData.getScoresArrayForNeighbor(n->getId()));
    }
  }
  // Then call the general method on these arrays:
  computeScoresFromArrays(iBitsets, iScores, rBitsets, rScores);
}

void DRTreeParsimonyScore::computeScoresForNode(const DRTreeParsimonyNodeData& pData, std::vector<Bitset>& rBitsets, std::vector<unsigned int>& rScores)
{
  const Node* node = pData.getNode();
  size_t nbNeighbors = node->degree();
  vector<const Node*> neighbors = node->getNeighbors();
  // First initialize the vectors fro input:
  vector< const vector<Bitset>*> iBitsets(nbNeighbors);
  vector< const vector<unsigned int>*> iScores(nbNeighbors);
  for (unsigned int k = 0; k < nbNeighbors; k++)
  {
    const Node* n = neighbors[k];
    iBitsets[k] =  &pData.getBitsetsArrayForNeighbor(n->getId());
    iScores [k] =  &pData.getScoresArrayForNeighbor(n->getId());
  }
  // Then call the general method on these arrays:
  computeScoresFromArrays(iBitsets, iScores, rBitsets, rScores);
}

/******************************************************************************/
unsigned int DRTreeParsimonyScore::getScore() const
{
  unsigned int score = 0;
  for (unsigned int i = 0; i < nbDistinctSites_; i++)
  {
    score += parsimonyData_->getRootScore(i) * parsimonyData_->getWeight(i);
  }
  return score;
}

/******************************************************************************/
unsigned int DRTreeParsimonyScore::getScoreForSite(size_t site) const
{
  return parsimonyData_->getRootScore(parsimonyData_->getRootArrayPosition(site));
}

/******************************************************************************/
void DRTreeParsimonyScore::computeScoresFromArrays(
    const vector< const vector<Bitset>*>& iBitsets,
    const vector< const vector<unsigned int>*>& iScores,
    vector<Bitset>& oBitsets,
    vector<unsigned int>& oScores)
{
  size_t nbPos  = oBitsets.size();
  size_t nbNodes = iBitsets.size();
  if (iScores.size() != nbNodes)
    throw Exception("DRTreeParsimonyScore::computeScores(); Error, input arrays must have the same length.");
  if (nbNodes < 1)
    throw Exception("DRTreeParsimonyScore::computeScores(); Error, input arrays must have a size >= 1.");
  const vector<Bitset>* bitsets0 = iBitsets[0];
  const vector<unsigned int>* scores0 = iScores[0];
  for (size_t i = 0; i < nbPos; i++)
  {
    oBitsets[i] = (*bitsets0)[i];
    oScores[i]  = (*scores0)[i];
  }
  for (size_t k = 1; k < nbNodes; k++)
  {
    const vector<Bitset>* bitsetsk = iBitsets[k];
    const vector<unsigned int>* scoresk = iScores[k];
    for (unsigned int i = 0; i < nbPos; i++)
    {
      Bitset bs = oBitsets[i] & (*bitsetsk)[i];
      oScores[i] += (*scoresk)[i];
      if (bs == 0)
      {
        bs = oBitsets[i] | (*bitsetsk)[i];
        oScores[i] += 1;
      }
      oBitsets[i] = bs;
    }
  }
}

/******************************************************************************/
double DRTreeParsimonyScore::testNNI(int nodeId) const
{
  const Node* son = treeTemplate().getNode(nodeId);
  if (!son->hasFather())
    throw NodePException("DRTreeParsimonyScore::testNNI(). Node 'son' must not be the root node.", son);
  const Node* parent = son->getFather();
  if (!parent->hasFather())
    throw NodePException("DRTreeParsimonyScore::testNNI(). Node 'parent' must not be the root node.", parent);
  const Node* grandFather = parent->getFather();
  // From here: Bifurcation assumed.
  // In case of multifurcation, an arbitrary uncle is chosen.
  // If we are at root node with a trifurcation, this does not matter, since 2 NNI are possible (see doc of the NNISearchable interface).
  size_t parentPosition = grandFather->getSonPosition(parent);
  const Node* uncle = grandFather->getSon(parentPosition > 1 ? parentPosition - 1 : 1 - parentPosition);

  // Retrieving arrays of interest:
  const DRTreeParsimonyNodeData& parentData = parsimonyData_->nodeData(parent->getId());
  const vector<Bitset>* sonBitsets = &parentData.getBitsetsArrayForNeighbor(son->getId());
  const vector<unsigned int>* sonScores  = &parentData.getScoresArrayForNeighbor(son->getId());
  vector<const Node*> parentNeighbors = TreeTemplateTools::getRemainingNeighbors(parent, grandFather, son);
  size_t nbParentNeighbors = parentNeighbors.size();
  vector< const vector<Bitset>*> parentBitsets(nbParentNeighbors);
  vector< const vector<unsigned int>*> parentScores(nbParentNeighbors);
  for (unsigned int k = 0; k < nbParentNeighbors; k++)
  {
    const Node* n = parentNeighbors[k]; // This neighbor
    parentBitsets[k] = &parentData.getBitsetsArrayForNeighbor(n->getId());
    parentScores[k] = &parentData.getScoresArrayForNeighbor(n->getId());
  }

  const DRTreeParsimonyNodeData& grandFatherData = parsimonyData_->nodeData(grandFather->getId());
  const vector<Bitset>* uncleBitsets = &grandFatherData.getBitsetsArrayForNeighbor(uncle->getId());
  const vector<unsigned int>* uncleScores  = &grandFatherData.getScoresArrayForNeighbor(uncle->getId());
  vector<const Node*> grandFatherNeighbors = TreeTemplateTools::getRemainingNeighbors(grandFather, parent, uncle);
  size_t nbGrandFatherNeighbors = grandFatherNeighbors.size();
  vector< const vector<Bitset>*> grandFatherBitsets(nbGrandFatherNeighbors);
  vector< const vector<unsigned int>*> grandFatherScores(nbGrandFatherNeighbors);
  for (unsigned int k = 0; k < nbGrandFatherNeighbors; k++)
  {
    const Node* n = grandFatherNeighbors[k]; // This neighbor
    grandFatherBitsets[k] = &grandFatherData.getBitsetsArrayForNeighbor(n->getId());
    grandFatherScores[k] = &grandFatherData.getScoresArrayForNeighbor(n->getId());
  }

  // Compute arrays and scores for grand-father node:
  grandFatherBitsets.push_back(sonBitsets);
  grandFatherScores.push_back(sonScores);
  // Init arrays:
  vector<Bitset> gfBitsets(sonBitsets->size()); // All arrays supposed to have the same size!
  vector<unsigned int> gfScores(sonScores->size());
  // Fill arrays:
  computeScoresFromArrays(grandFatherBitsets, grandFatherScores, gfBitsets, gfScores);

  // Now computes arrays and scores for parent node:
  parentBitsets.push_back(uncleBitsets);
  parentScores.push_back(uncleScores);
  parentBitsets.push_back(&gfBitsets);
  parentScores.push_back(&gfScores);
  // Init arrays:
  vector<Bitset> pBitsets(sonBitsets->size()); // All arrays supposed to have the same size!
  vector<unsigned int> pScores(sonScores->size());
  // Fill arrays:
  computeScoresFromArrays(parentBitsets, parentScores, pBitsets, pScores);

  // Final computation:
  unsigned int score = 0;
  for (unsigned int i = 0; i < nbDistinctSites_; i++)
  {
    score += pScores[i] * parsimonyData_->getWeight(i);
  }
  return (double)score - (double)getScore();
}

/******************************************************************************/

void DRTreeParsimonyScore::doNNI(int nodeId)
{
  Node* son = treeTemplate_().getNode(nodeId);
  if (!son->hasFather())
    throw NodePException("DRTreeParsimonyScore::doNNI(). Node 'son' must not be the root node.", son);
  Node* parent = son->getFather();
  if (!parent->hasFather())
    throw NodePException("DRTreeParsimonyScore::doNNI(). Node 'parent' must not be the root node.", parent);
  Node* grandFather = parent->getFather();
  // From here: Bifurcation assumed.
  // In case of multifurcation, an arbitrary uncle is chosen.
  // If we are at root node with a trifurcation, this does not matter, since 2 NNI are possible (see doc of the NNISearchable interface).
  size_t parentPosition = grandFather->getSonPosition(parent);
  Node* uncle = grandFather->getSon(parentPosition > 1 ? parentPosition - 1 : 1 - parentPosition);
  // Swap nodes:
  parent->removeSon(son);
  grandFather->removeSon(uncle);
  parent->addSon(uncle);
  grandFather->addSon(son);
}

/******************************************************************************/

// /******************************************************************************/
//
// void DRTreeParsimonyScore::setNodeState(Node* node, size_t state)
// {
//     Number<size_t>* stateProperty = new Number<size_t>(state);
//     node->setNodeProperty(STATE, *stateProperty);
//     delete stateProperty;
// }
//
// /******************************************************************************/
//
// size_t DRTreeParsimonyScore::getNodeState(const Node* node)
// {
//   return (dynamic_cast<const Number<size_t>*>(node->getNodeProperty(STATE)))->getValue(); // exception on root on the true history - why didn't the root recieve a state?
// }
//
// /******************************************************************************/
//
// void DRTreeParsimonyScore::computeSolution()
// {
//   map< int, vector<size_t> > nodeToPossibleStates;
//   TreeTemplate<Node>* tree = getTreeP_();
//   vector<Node*> nodes = tree->getNodes();
//   vector<Bitset> nodeBitsets;
//   for (size_t n = 0; n < nodes.size(); ++n)
//   {
//     // extract the node's bisets (i.e, possible states assignments)
//     if (nodes[n]->isLeaf())
//     {
//       nodeBitsets = (&parsimonyData_->getLeafData(nodes[n]->getId()))->getBitsetsArray();
//     }
//     else if (nodes[n]->hasFather())
//     {
//       nodeBitsets = (&parsimonyData_->getNodeData(nodes[n]->getFather()->getId()))->getBitsetsArrayForNeighbor(nodes[n]->getId());  // extract the bitset corresponding to the son from its father's bitSet array
//     }
//     else // get the node's possible states from its first internal neighbor
//     {
//       int neighborId = 0;
//       vector<Node*> neighbors = nodes[n]->getNeighbors();
//       for (unsigned int nn=0; nn<neighbors.size(); ++nn)
//       {
//         if (!neighbors[nn]->isLeaf())
//         {
//           neighborId = neighbors[nn]->getId();
//           break;
//         }
//       }
//        nodeBitsets = (&parsimonyData_->getNodeData(neighborId))->getBitsetsArrayForNeighbor(nodes[n]->getId());
//     }
//     // map the node id to its possible states
//     vector <size_t> possibleStates;
//     for (size_t s = 0; s < getStateMap().getNumberOfModelStates(); ++s)
//     {
//       if (nodeBitsets[0].test(s))
//       {
//         possibleStates.push_back(s);
//       }
//     }
//     nodeToPossibleStates[nodes[n]->getId()] = possibleStates;
//   }
//
//   // set states for the nodes according to their possible assignments and parent state
//   TreeIterator* treeIt = new PreOrderTreeIterator(*tree);
//   for (const Node* node = treeIt->begin(); node != treeIt->end(); node = treeIt->next())
//   {
//     size_t nodeState;
//     vector<size_t> possibleStates = nodeToPossibleStates[node->getId()];
//     if (possibleStates.size() == 1)
//     {
//       nodeState = possibleStates[0];
//     }
//     else if (node->hasFather()) // if the node has a father -> set its state according to its father's state
//     {
//       nodeState = getNodeState(node->getFather());
//     }
//     else                       // if there is no restriction from the father -> set the state randomely
//     {
//       nodeState = RandomTools::pickOne(possibleStates);
//     }
//     setNodeState(tree->getNode(node->getId()), nodeState);
//   }
//   delete treeIt;
// }
