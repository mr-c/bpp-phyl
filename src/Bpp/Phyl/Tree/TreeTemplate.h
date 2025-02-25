// SPDX-FileCopyrightText: The Bio++ Development Group
//
// SPDX-License-Identifier: CECILL-2.1

#ifndef BPP_PHYL_TREE_TREETEMPLATE_H
#define BPP_PHYL_TREE_TREETEMPLATE_H


#include "Tree.h"
#include "TreeExceptions.h"
#include "TreeTemplateTools.h"

// From the STL:
#include <string>
#include <vector>
#include <map>

namespace bpp
{
/**
 * @brief The phylogenetic tree class.
 *
 * This class is part of the object implementation of phylogenetic trees. Tree are made
 * made of nodes, instances of the class Node. It is possible to use a tree with more
 * complexe Node classes, but currently all nodes of a tree have to be of the same class.
 *
 * Trees are oriented (rooted), i.e. each node has one <i>father node</i> and possibly
 * many <i>son nodes</i>. Leaves are nodes without descendant and root is defined has the without
 * father. Inner nodes will generally contain two descendants (the tree is then called
 * <i>bifurcating</i>), but mutlifurcating trees are also allowed with this kind of description.
 * In the rooted case, each inner node also defines a <i>subtree</i>.
 * This allows to work recursively on trees, which is very convenient in most cases.
 * To deal with non-rooted trees, we place an artificial root at a particular node:
 * hence the root node appears to be trifurcated. This is the way unrooted trees are
 * described in the parenthetic description, the so called Newick format.
 *
 * To clone a tree from from another tree with a different template,
 * consider using the TreeTools::cloneSutree<N>() method:
 * @code
 * Tree * t = new Tree<Node>(...)
 * NodeTemplate<int> * newRoot = TreeTools::cloneSubtree< NodeTemplate<int> >(* (t -> getRootNode()))
 * Tree< NodeTemplate<int> > * tt = new Tree< NodeTemplate<int> >(* newRoot);
 * @endcode
 *
 * The getNextId() method sends a id value which is not used in the tree.
 * In the current implementation, it uses the TreeTools::getMPNUId() method.
 * This avoids to use duplicated ids, but is time consuming.
 * In most cases, it is of better efficiency if the user deal with the ids himself, by using the Node::setId() method.
 * The TreeTools::getMaxId() method may also prove useful in this respect.
 * The resetNodesId() method can also be used to re-initialize all ids.
 *
 * @see Node
 * @see NodeTemplate
 * @see TreeTools
 */
template<class N>
class TreeTemplate :
  public Tree
{
  /**
   * Fields:
   */

private:
  N* root_;
  std::string name_;

public:
  // Constructors and destructor:
  TreeTemplate() : root_(0),
    name_() {}

  TreeTemplate(const TreeTemplate<N>& t) :
    root_(0),
    name_(t.name_)
  {
    // Perform a hard copy of the nodes:
    root_ = TreeTemplateTools::cloneSubtree<N>(*t.getRootNode());
  }

  TreeTemplate(const Tree& t) :
    root_(0),
    name_(t.getName())
  {
    // Create new nodes from an existing tree:
    root_ = TreeTemplateTools::cloneSubtree<N>(t, t.getRootId());
  }

  TreeTemplate(N* root) : root_(root),
    name_()
  {
    root_->removeFather(); // In case this is a subtree from somewhere else...
  }

  TreeTemplate<N>& operator=(const TreeTemplate<N>& t)
  {
    // Perform a hard copy of the nodes:
    if (root_) { TreeTemplateTools::deleteSubtree(root_); delete root_; }
    root_ = TreeTemplateTools::cloneSubtree<N>(*t.getRootNode());
    name_ = t.name_;
    return *this;
  }

  TreeTemplate<N>* cloneSubtree(int newRootId) const
  {
    N* newRoot = TreeTemplateTools::cloneSubtree<N>(*this, newRootId);
    return new TreeTemplate<N>(newRoot);
  }

  virtual ~TreeTemplate()
  {
    TreeTemplateTools::deleteSubtree(root_);
    delete root_;
  }

  TreeTemplate<N>* clone() const { return new TreeTemplate<N>(*this); }

  /**
   * Methods:
   */

public:
  std::string getName() const { return name_; }

  void setName(const std::string& name) { name_ = name; }

  int getRootId() const { return root_->getId(); }

  size_t getNumberOfLeaves() const { return TreeTemplateTools::getNumberOfLeaves(*root_); }

  size_t getNumberOfNodes() const { return TreeTemplateTools::getNumberOfNodes(*root_); }

  size_t getNumberOfBranches() const { return TreeTemplateTools::getNumberOfBranches(*root_); }

  int getLeafId(const std::string& name) const { return TreeTemplateTools::getLeafId(*root_, name); }

  std::vector<int> getLeavesId() const { return TreeTemplateTools::getLeavesId(*root_); }

  std::vector<int> getNodesId() const { return TreeTemplateTools::getNodesId(*root_); }

  std::vector<int> getInnerNodesId() const { return TreeTemplateTools::getInnerNodesId(*root_); }

  std::vector<int> getBranchesId() const
  {
    Vint vRes = TreeTemplateTools::getNodesId(*root_);
    int rId = getRootId();
    std::vector<int>::iterator rit(vRes.begin());
    while (rit < vRes.end())
      if (*rit == rId)
      {
        vRes.erase(rit, rit + 1);
        return vRes;
      }
      else
        rit++;

    return vRes;
  }

  std::vector<double> getBranchLengths() const { return TreeTemplateTools::getBranchLengths(*root_); }

  std::vector<std::string> getLeavesNames() const { return TreeTemplateTools::getLeavesNames(*const_cast<const N*>( root_)); }

  std::vector<int> getSonsId(int parentId) const { return getNode(parentId)->getSonsId(); }

  std::vector<int> getAncestorsId(int nodeId) const { return TreeTemplateTools::getAncestorsId(*getNode(nodeId)); }

  int getFatherId(int parentId) const { return getNode(parentId)->getFatherId(); }

  bool hasFather(int nodeId) const { return getNode(nodeId)->hasFather(); }

  std::string getNodeName(int nodeId) const { return getNode(nodeId)->getName(); }

  bool hasNodeName(int nodeId) const { return getNode(nodeId)->hasName(); }

  void setNodeName(int nodeId, const std::string& name) { getNode(nodeId)->setName(name); }

  void deleteNodeName(int nodeId) { return getNode(nodeId)->deleteName(); }

  bool hasNode(int nodeId) const { return TreeTemplateTools::hasNodeWithId(*root_, nodeId); }

  bool isLeaf(int nodeId) const { return getNode(nodeId)->isLeaf(); }

  bool hasNoSon(int nodeId) const { return getNode(nodeId)->hasNoSon(); }

  bool isRoot(int nodeId) const { return TreeTemplateTools::isRoot(*getNode(nodeId)); }

  double getDistanceToFather(int nodeId) const { return getNode(nodeId)->getDistanceToFather(); }

  void setDistanceToFather(int nodeId, double length) { getNode(nodeId)->setDistanceToFather(length); }

  void deleteDistanceToFather(int nodeId) { getNode(nodeId)->deleteDistanceToFather(); }

  bool hasDistanceToFather(int nodeId) const { return getNode(nodeId)->hasDistanceToFather(); }

  bool hasNodeProperty(int nodeId, const std::string& name) const { return getNode(nodeId)->hasNodeProperty(name); }

  void setNodeProperty(int nodeId, const std::string& name, const Clonable& property) { getNode(nodeId)->setNodeProperty(name, property); }

  Clonable* getNodeProperty(int nodeId, const std::string& name) { return getNode(nodeId)->getNodeProperty(name); }

  const Clonable* getNodeProperty(int nodeId, const std::string& name) const { return getNode(nodeId)->getNodeProperty(name); }

  Clonable* removeNodeProperty(int nodeId, const std::string& name) { return getNode(nodeId)->removeNodeProperty(name); }

  std::vector<std::string> getNodePropertyNames(int nodeId) const { return getNode(nodeId)->getNodePropertyNames(); }

  bool hasBranchProperty(int nodeId, const std::string& name) const { return getNode(nodeId)->hasBranchProperty(name); }

  void setBranchProperty(int nodeId, const std::string& name, const Clonable& property) { getNode(nodeId)->setBranchProperty(name, property); }

  Clonable* getBranchProperty(int nodeId, const std::string& name) { return getNode(nodeId)->getBranchProperty(name); }

  const Clonable* getBranchProperty(int nodeId, const std::string& name) const { return getNode(nodeId)->getBranchProperty(name); }

  Clonable* removeBranchProperty(int nodeId, const std::string& name) { return getNode(nodeId)->removeBranchProperty(name); }

  std::vector<std::string> getBranchPropertyNames(int nodeId) const { return getNode(nodeId)->getBranchPropertyNames(); }

  void rootAt(int nodeId) { rootAt(getNode(nodeId)); }

  void newOutGroup(int nodeId) {  newOutGroup(getNode(nodeId)); }

  bool isRooted() const { return root_->getNumberOfSons() == 2; }

  bool unroot()
  {
    if (!isRooted()) throw UnrootedTreeException("Tree::unroot", this);
    else
    {
      N* son1 = dynamic_cast<N*>(root_->getSon(0));
      N* son2 = dynamic_cast<N*>(root_->getSon(1));
      if (son1->isLeaf() && son2->isLeaf()) return false; // We can't unroot a single branch!

      // We manage to have a subtree in position 0:
      if (son1->isLeaf())
      {
        root_->swap(0, 1);
        son1 = dynamic_cast<N*>(root_->getSon(0));
        son2 = dynamic_cast<N*>(root_->getSon(1));
      }

      // Take care of branch lengths:
      if (son1->hasDistanceToFather())
      {
        if (son2->hasDistanceToFather())
        {
          // Both nodes have lengths, we sum them:
          son2->setDistanceToFather(son1->getDistanceToFather() + son2->getDistanceToFather());
        }
        else
        {
          // Only node 1 has length, we set it to node 2:
          son2->setDistanceToFather(son1->getDistanceToFather());
        }
        son1->deleteDistanceToFather();
      } // Else node 2 may or may not have a branch length, we do not care!

      // Remove the root:
      root_->removeSons();
      son1->addSon(son2);
      delete root_;
      setRootNode(son1);
      return true;
    }
  }

  void resetNodesId()
  {
    std::vector<N*> nodes = getNodes();
    for (size_t i = 0; i < nodes.size(); i++)
    {
      nodes[i]->setId(static_cast<int>(i));
    }
  }

  bool isMultifurcating() const
  {
    if (root_->getNumberOfSons() > 3) return true;
    for (size_t i = 0; i < root_->getNumberOfSons(); i++)
    {
      if (TreeTemplateTools::isMultifurcating(*root_->getSon(i)))
        return true;
    }
    return false;
  }

  /**
   * @brief Tells if this tree has the same topology as the one given for comparison.
   *
   * This method compares recursively all subtrees. The comparison is performed only on the nodes names and the parental relationships.
   * Nodes ids are ignored, and so are branch lengths and any branch/node properties. The default is to ignore the ordering of the descendants,
   * that is (A,B),C) will be considered as having the same topology as (B,A),C). Multifurcations are permited.
   * If ordering is ignored, a copy of the two trees to be compared is performed and are ordered before comparison, making the whole comparison
   * slower and more memory greedy.
   *
   * @param tree The tree to be compared with.
   * @param ordered Should the ordering of the branching be taken into account?
   * @return True if the input tree has the same topology as this one.
   */
  template<class N2>
  bool hasSameTopologyAs(const TreeTemplate<N2>& tree, bool ordered = false) const
  {
    const TreeTemplate<N>* t1 = 0;
    const TreeTemplate<N2>* t2 = 0;
    if (ordered)
    {
      t1 = this;
      t2 = &tree;
    }
    else
    {
      TreeTemplate<N>* t1tmp = this->clone();
      TreeTemplate<N2>* t2tmp = tree.clone();
      TreeTemplateTools::orderTree(*t1tmp->getRootNode(), true, true);
      TreeTemplateTools::orderTree(*t2tmp->getRootNode(), true, true);
      t1 = t1tmp;
      t2 = t2tmp;
    }
    bool test = TreeTemplateTools::haveSameOrderedTopology(*t1->getRootNode(), *t2->getRootNode());
    if (!ordered)
    {
      delete t1;
      delete t2;
    }
    return test;
  }

  std::vector<double> getBranchLengths()
  {
    Vdouble brLen(1);
    for (size_t i = 0; i < root_->getNumberOfSons(); i++)
    {
      Vdouble sonBrLen = TreeTemplateTools::getBranchLengths(*root_->getSon(i));
      for (size_t j = 0; j < sonBrLen.size(); j++) { brLen.push_back(sonBrLen[j]); }
    }
    return brLen;
  }

  double getTotalLength()
  {
    return TreeTemplateTools::getTotalLength(*root_, false);
  }

  void setBranchLengths(double brLen)
  {
    for (size_t i = 0; i < root_->getNumberOfSons(); i++)
    {
      TreeTemplateTools::setBranchLengths(*root_->getSon(i), brLen);
    }
  }

  void setVoidBranchLengths(double brLen)
  {
    for (size_t i = 0; i < root_->getNumberOfSons(); i++)
    {
      TreeTemplateTools::setVoidBranchLengths(*root_->getSon(i), brLen);
    }
  }

  void scaleTree(double factor)
  {
    for (size_t i = 0; i < root_->getNumberOfSons(); i++)
    {
      TreeTemplateTools::scaleTree(*root_->getSon(i), factor);
    }
  }

  int getNextId()
  {
    return TreeTools::getMPNUId(*this, root_->getId());
  }

  void swapNodes(int parentId, size_t i1, size_t i2)
  {
    std::vector<N*> nodes = TreeTemplateTools::searchNodeWithId<N>(*root_, parentId);
    if (nodes.size() == 0) throw NodeNotFoundException("TreeTemplate:swapNodes(): Node with id not found.", TextTools::toString(parentId));
    for (size_t i = 0; i < nodes.size(); i++) { nodes[i]->swap(i1, i2); }
  }


  /**
   * @name Specific methods
   *
   * @{
   */
  virtual void setRootNode(N* root) { root_ = root; root_->removeFather(); }

  virtual N* getRootNode() { return root_; }

  virtual const N* getRootNode() const { return root_; }

  virtual std::vector<const N*> getLeaves() const { return TreeTemplateTools::getLeaves(*const_cast<const N*>(root_)); }

  virtual std::vector<N*> getLeaves() { return TreeTemplateTools::getLeaves(*root_); }

  virtual std::vector<const N*> getNodes() const { return TreeTemplateTools::getNodes(*const_cast<const N*>(root_)); }

  virtual std::vector<N*> getNodes() { return TreeTemplateTools::getNodes(*root_); }

  virtual std::vector<const N*> getInnerNodes() const { return TreeTemplateTools::getInnerNodes(*const_cast<const N*>(root_)); }

  virtual std::vector<N*> getInnerNodes() { return TreeTemplateTools::getInnerNodes(*root_); }

  virtual N* getNode(int id, bool checkId = false)
  {
    if (checkId)
    {
      std::vector<N*> nodes;
      TreeTemplateTools::searchNodeWithId<N>(*dynamic_cast<N*>(root_), id, nodes);
      if (nodes.size() > 1) throw Exception("TreeTemplate::getNode(): Non-unique id! (" + TextTools::toString(id) + ").");
      if (nodes.size() == 0) throw NodeNotFoundException("TreeTemplate::getNode(): Node with id not found.", TextTools::toString(id));
      return nodes[0];
    }
    else
    {
      N* node = dynamic_cast<N*>(TreeTemplateTools::searchFirstNodeWithId(*root_, id));
      if (node)
        return node;
      else
        throw NodeNotFoundException("TreeTemplate::getNode(): Node with id not found.", TextTools::toString(id));
    }
  }

  virtual const N* getNode(int id, bool checkId = false) const
  {
    if (checkId)
    {
      std::vector<const N*> nodes;
      TreeTemplateTools::searchNodeWithId<const N>(*root_, id, nodes);
      if (nodes.size() > 1) throw Exception("TreeTemplate::getNode(): Non-unique id! (" + TextTools::toString(id) + ").");
      if (nodes.size() == 0) throw NodeNotFoundException("TreeTemplate::getNode(): Node with id not found.", TextTools::toString(id));
      return nodes[0];
    }
    else
    {
      const N* node = dynamic_cast<const N*>(TreeTemplateTools::searchFirstNodeWithId(*root_, id));
      if (node)
        return node;
      else
        throw NodeNotFoundException("TreeTemplate::getNode(): Node with id not found.", TextTools::toString(id));
    }
  }

  virtual N* getNode(const std::string& name)
  {
    std::vector<N*> nodes;
    TreeTemplateTools::searchNodeWithName(*root_, name, nodes);
    if (nodes.size() > 1) throw NodeNotFoundException("TreeTemplate::getNode(): Non-unique name.", "" + name);
    if (nodes.size() == 0) throw NodeNotFoundException("TreeTemplate::getNode(): Node with name not found.", "" + name);
    return nodes[0];
  }

  virtual const N* getNode(const std::string& name) const
  {
    std::vector<const N*> nodes;
    TreeTemplateTools::searchNodeWithName<const N>(*root_, name, nodes);
    if (nodes.size() > 1) throw NodeNotFoundException("TreeTemplate::getNode(): Non-unique name.", "" + name);
    if (nodes.size() == 0) throw NodeNotFoundException("TreeTemplate::getNode(): Node with name not found.", "" + name);
    return nodes[0];
  }

  void rootAt(N* newRoot)
  {
    if (root_ == newRoot) return;
    if (isRooted()) unroot();
    std::vector<Node*> path = TreeTemplateTools::getPathBetweenAnyTwoNodes(*root_, *newRoot);
    for (size_t i = 0; i < path.size() - 1; i++)
    {
      if (path[i + 1]->hasDistanceToFather())
      {
        path[i]->setDistanceToFather(path[i + 1]->getDistanceToFather());
      }
      else path[i]->deleteDistanceToFather();
      path[i]->removeSon(path[i + 1]);
      path[i + 1]->addSon(path[i]);

      std::vector<std::string> names = path[i + 1]->getBranchPropertyNames();
      for (size_t j = 0; j < names.size(); j++)
      {
        path[i]->setBranchProperty(names[j], *path[i + 1]->getBranchProperty(names[j]));
      }
      path[i + 1]->deleteBranchProperties();
    }

    newRoot->deleteDistanceToFather();
    newRoot->deleteBranchProperties();
    root_ = newRoot;
  }

  void newOutGroup(N* outGroup)
  {
    if (root_ == outGroup) return;
    int rootId;
    if (isRooted())
    {
      for (size_t i = 0; i < root_->getNumberOfSons(); i++)
      {
        if (root_->getSon(i) == outGroup) return; // This tree is already rooted appropriately.
      }
      rootId = getRootId();
      unroot();
    }
    else
    {
      rootId = getNextId();
    }
    rootAt(dynamic_cast<N*>(outGroup->getFather()));
    N* oldRoot = root_;
    oldRoot->removeSon(outGroup);
    root_ = new N();
    root_->setId(rootId);
    root_->addSon(oldRoot);
    root_->addSon(outGroup);
    // Check lengths:
    if (outGroup->hasDistanceToFather())
    {
      double l = outGroup->getDistanceToFather() / 2.;
      outGroup->setDistanceToFather(l);
      oldRoot->setDistanceToFather(l);
    }
  }

  /** @} */
};
} // end of namespace bpp.
#endif // BPP_PHYL_TREE_TREETEMPLATE_H
