// SPDX-FileCopyrightText: The Bio++ Development Group
//
// SPDX-License-Identifier: CECILL-2.1

#ifndef BPP_PHYL_GRAPHICS_ABSTRACTTREEDRAWING_H
#define BPP_PHYL_GRAPHICS_ABSTRACTTREEDRAWING_H


#include "../Tree/NodeTemplate.h"
#include "../Tree/TreeTemplate.h"
#include "TreeDrawing.h"
#include "TreeDrawingListener.h"

// From ths STL:
#include <vector>
#include <string>
#include <memory>

namespace bpp
{
class TreeDrawingNodeInfo
{
private:
  Point2D<double> pos_;
  bool collapsed_;

public:
  TreeDrawingNodeInfo() : pos_(), collapsed_(false) {}
  virtual ~TreeDrawingNodeInfo() {}

public:
  const Point2D<double>& getPosition() const { return pos_; }
  Point2D<double>& getPosition() { return pos_; }
  void setPosition(const Point2D<double>& position) { pos_ = position; }
  double getX() const { return pos_.getX(); }
  double getY() const { return pos_.getY(); }
  void setX(double x) { pos_.setX(x); }
  void setY(double y) { pos_.setY(y); }
  void collapse(bool yn) { collapsed_ = yn; }
  bool isCollapsed() const { return collapsed_; }
};

typedef NodeTemplate<TreeDrawingNodeInfo> INode;


/**
 * @brief Event class that uses INode object (more efficient than relying on nodes id, but less generic).
 */
class DrawINodeEvent :
  public DrawNodeEvent
{
private:
  const INode* node_;

public:
  DrawINodeEvent(const TreeDrawing* source, GraphicDevice* gd, const INode* node, const Cursor& cursor) :
    DrawNodeEvent(source, gd, node->getId(), cursor),
    node_(node)
  {}

  DrawINodeEvent(const DrawINodeEvent& dne) :
    DrawNodeEvent(dne), node_(dne.node_)
  {}

  DrawINodeEvent& operator=(const DrawINodeEvent& dne)
  {
    DrawNodeEvent::operator=(dne);
    node_ = dne.node_;
    return *this;
  }

public:
  const INode* getINode() const { return node_; }
};


/**
 * @brief Event class that uses INode object (more efficient than relying on nodes id, but less generic).
 */
class DrawIBranchEvent :
  public DrawBranchEvent
{
private:
  const INode* node_;

public:
  DrawIBranchEvent(const TreeDrawing* source, GraphicDevice* gd, const INode* node, const Cursor& cursor) :
    DrawBranchEvent(source, gd, node->getId(), cursor),
    node_(node)
  {}

  DrawIBranchEvent(const DrawIBranchEvent& dne) :
    DrawBranchEvent(dne), node_(dne.node_)
  {}

  DrawIBranchEvent& operator=(const DrawIBranchEvent& dne)
  {
    DrawBranchEvent::operator=(dne);
    node_ = dne.node_;
    return *this;
  }

public:
  const INode* getINode() const { return node_; }
};


/**
 * @brief Partial implementation of the TreeDrawing interface.
 *
 * This basic implementation uses a dedicated NodeInfo structure in combination with the NodeTemplate class.
 * This structures stores the current coordinates of all nodes, so that it is easy to annotate the tree drawing.:if expand("%") == ""|browse confirm w|else|confirm w|endif
 *
 */
class AbstractTreeDrawing :
  public virtual TreeDrawing
{
private:
  std::unique_ptr<TreeTemplate<INode>> tree_;
  double xUnit_;
  double yUnit_;
  const TreeDrawingSettings* settings_;
  std::vector<TreeDrawingListener*> listeners_;

public:
  AbstractTreeDrawing() : tree_(), xUnit_(1.), yUnit_(1.), settings_(&DEFAULT_SETTINGS), listeners_() {}

  AbstractTreeDrawing(const AbstractTreeDrawing& atd) :
    tree_(atd.tree_.get() ? dynamic_cast<TreeTemplate<INode>*>(atd.tree_->clone()) : 0),
    xUnit_(atd.xUnit_),
    yUnit_(atd.yUnit_),
    settings_(atd.settings_),
    listeners_(atd.listeners_.size())
  {
    for (unsigned int i = 0; i < listeners_.size(); ++i)
    {
      if (atd.listeners_[i]->isAutonomous())
        listeners_[i] = atd.listeners_[i];
      else
        listeners_[i] = dynamic_cast<TreeDrawingListener*>(atd.listeners_[i]->clone());
    }
  }

  AbstractTreeDrawing& operator=(const AbstractTreeDrawing& atd)
  {
    if (atd.tree_.get())
      tree_.reset(dynamic_cast<TreeTemplate<INode>*>(atd.tree_->clone()));
    else tree_.reset();
    xUnit_              = atd.xUnit_;
    yUnit_              = atd.yUnit_;
    settings_           = atd.settings_;
    listeners_.resize(atd.listeners_.size());
    for (unsigned int i = 0; i < listeners_.size(); ++i)
    {
      if (atd.listeners_[i]->isAutonomous())
        listeners_[i] = atd.listeners_[i];
      else
        listeners_[i] = dynamic_cast<TreeDrawingListener*>(atd.listeners_[i]->clone());
    }
    return *this;
  }

  virtual ~AbstractTreeDrawing()
  {
    for (unsigned int i = 0; i < listeners_.size(); i++)
    {
      if (!listeners_[i]->isAutonomous())
        delete listeners_[i];
    }
  }

public:
  bool hasTree() const { return tree_.get() != 0; }

  const TreeTemplate<INode>* getTree() const { return tree_.get(); }

  void setTree(const Tree* tree)
  {
    if (tree_.get())
      tree_.reset();
    if (!tree) tree_.reset();
    else
    {
      tree_.reset(new TreeTemplate<INode>(*tree)); // We copy the tree
    }
    treeHasChanged();
  }

  Point2D<double> getNodePosition(int nodeId) const;

  int getNodeAt(const Point2D<double>& position) const;

  /**
   * @brief Utilitary function, telling if a point belongs to a specified area.
   *
   * This method is used internally to get a node coordinates.
   *
   * @param p1 Point to look for.
   * @param p2 Second point defining the center of the area.
   * @return True if p1 belongs to the area defined by p2.
   */
  bool belongsTo(const Point2D<double>& p1, const Point2D<double>& p2) const;

  /**
   * @brief Draw some text at a particular node position.
   *
   * @param gDevice The GraphicDevice object on which to draw.
   * @param node The node of interest.
   * @param text The text to draw.
   * @param xOffset Horizontal offset.
   * @param yOffset Vertical offset.
   * @param hpos The way the text should be aligned horizontally (see GraphicDevice).
   * @param vpos The way the text should be aligned vertically (see GraphicDevice).
   * @param angle The rotation value of the text.
   */
  virtual void drawAtNode(GraphicDevice& gDevice, const INode& node, const std::string& text,
      double xOffset = 0, double yOffset = 0,
      short hpos = GraphicDevice::TEXT_HORIZONTAL_LEFT, short vpos = GraphicDevice::TEXT_VERTICAL_CENTER, double angle = 0) const;

  /**
   * @brief Draw some text at a particular branch position.
   *
   * @param gDevice The GraphicDevice object on which to draw.
   * @param node The node of interest.
   * @param text The text to draw.
   * @param xOffset Horizontal offset.
   * @param yOffset Vertical offset.
   * @param hpos The way the text should be aligned horizontally (see GraphicDevice).
   * @param vpos The way the text should be aligned vertically (see GraphicDevice).
   * @param angle The rotation value of the text.
   */
  virtual void drawAtBranch(GraphicDevice& gDevice, const INode& node, const std::string& text, double xOffset = 0, double yOffset = 0, short hpos = GraphicDevice::TEXT_HORIZONTAL_LEFT, short vpos = GraphicDevice::TEXT_VERTICAL_CENTER, double angle = 0) const;

  void setDisplaySettings(const TreeDrawingSettings* tds)
  {
    if (!tds)
      throw NullPointerException("AbstractTreeDrawing::setDisplaySettings. Null pointer provided.");
    settings_ = tds;
  }
  const TreeDrawingSettings& getDisplaySettings() const { return *settings_; }

  double getXUnit() const { return xUnit_; }

  double getYUnit() const { return yUnit_; }

  void setXUnit(double xu) { xUnit_ = xu; }

  void setYUnit(double yu) { yUnit_ = yu; }

  void collapseNode(int nodeId, bool yn)
  {
    if (!tree_.get()) throw Exception("AbstractTreeDrawing::collapseNode. No tree is associated to the drawing.");
    tree_->getNode(nodeId)->getInfos().collapse(yn);
  }

  bool isNodeCollapsed(int nodeId) const
  {
    if (!tree_.get()) throw Exception("AbstractTreeDrawing::isNodeCollapsed. No tree is associated to the drawing.");
    return tree_->getNode(nodeId)->getInfos().isCollapsed();
  }

  void addTreeDrawingListener(TreeDrawingListener* listener)
  {
    if (find(listeners_.begin(), listeners_.end(), listener) != listeners_.end())
      throw Exception("AbstractTreeDrawing::addTreeDrawingListener. Listener is already associated to this drawing.");
    listeners_.push_back(listener);
  }

  void removeTreeDrawingListener(TreeDrawingListener* listener)
  {
    std::vector<TreeDrawingListener*>::iterator it = std::find(listeners_.begin(), listeners_.end(), listener);
    if (it == listeners_.end())
      throw Exception("AbstractTreeDrawing::addTreeDrawingListener. Listener is not associated to this drawing, and therefore can't be removed.");
    listeners_.erase(it);
    if (!listener->isAutonomous())
      delete listener;
  }

  /**
   * @brief Method to implement to deal with redrawing when the underlying tree has been modified.
   */
  virtual void treeHasChanged() = 0;

protected:
  TreeTemplate<INode>* getTree_() { return tree_.get(); }
  const TreeTemplate<INode>* getTree_() const { return tree_.get(); }

  void fireBeforeTreeEvent_(const DrawTreeEvent& event) const
  {
    for (unsigned int i = 0; i < listeners_.size(); i++)
    {
      if (listeners_[i]->isEnabled())
        listeners_[i]->beforeDrawTree(event);
    }
  }

  void fireAfterTreeEvent_(const DrawTreeEvent& event) const
  {
    for (unsigned int i = 0; i < listeners_.size(); i++)
    {
      if (listeners_[i]->isEnabled())
        listeners_[i]->afterDrawTree(event);
    }
  }

  void fireBeforeNodeEvent_(const DrawINodeEvent& event) const
  {
    for (unsigned int i = 0; i < listeners_.size(); i++)
    {
      if (listeners_[i]->isEnabled())
        listeners_[i]->beforeDrawNode(event);
    }
  }

  void fireAfterNodeEvent_(const DrawINodeEvent& event) const
  {
    for (unsigned int i = 0; i < listeners_.size(); i++)
    {
      if (listeners_[i]->isEnabled())
        listeners_[i]->afterDrawNode(event);
    }
  }

  void fireBeforeBranchEvent_(const DrawIBranchEvent& event) const
  {
    for (unsigned int i = 0; i < listeners_.size(); i++)
    {
      if (listeners_[i]->isEnabled())
        listeners_[i]->beforeDrawBranch(event);
    }
  }

  void fireAfterBranchEvent_(const DrawIBranchEvent& event) const
  {
    for (unsigned int i = 0; i < listeners_.size(); i++)
    {
      if (listeners_[i]->isEnabled())
        listeners_[i]->afterDrawBranch(event);
    }
  }

public:
  static const TreeDrawingSettings DEFAULT_SETTINGS;
};
} // end of namespace bpp.
#endif // BPP_PHYL_GRAPHICS_ABSTRACTTREEDRAWING_H
