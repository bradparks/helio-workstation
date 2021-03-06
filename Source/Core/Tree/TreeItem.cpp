/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Common.h"
#include "TreeItem.h"
#include "TreePanel.h"
#include "TreeItemComponentDefault.h"
#include "TreeItemComponentCompact.h"
#include "MidiLayerOwner.h"
#include "MainLayout.h"
#include "HelioCallout.h"

String TreeItem::createSafeName(const String &nameStr)
{
    return File::createLegalFileName(nameStr).removeCharacters(MidiLayerOwner::xPathSeparator);
}

TreeItem::TreeItem(const String &nameStr) :
    name(TreeItem::createSafeName(nameStr)),
    markerIsVisible(false),
    itemShouldBeVisible(true),
    itemIsGreyedOut(false)
{

//#if HELIO_DESKTOP
    this->setLinesDrawnForSubItems(false);
//#elif HELIO_MOBILE
//    this->setLinesDrawnForSubItems(true);
//#endif

    this->setDrawsInLeftMargin(true);
}

TreeItem::~TreeItem()
{
    // удаляем детей
    this->deleteAllSubItems();

    // удаляем себя из дерева
    this->removeItemFromParent();

    this->masterReference.clear();
}

String TreeItem::getUniqueName() const
{
    return String(this->getIndexInParent());
}

TreePanel *TreeItem::findParentTreePanel() const
{
    return this->getOwnerView()->findParentComponentOfClass<TreePanel>();
}

void TreeItem::setMarkerVisible(bool shouldBeVisible) noexcept
{
    this->markerIsVisible = shouldBeVisible;
}

bool TreeItem::isMarkerVisible() const noexcept
{
    return this->markerIsVisible;
}

void TreeItem::setGreyedOut(bool shouldBeGreyedOut) noexcept
{
//    this->setSelected(! shouldBeGreyedOut, false);

    this->setSelected(false, false);

    if (this->itemIsGreyedOut != shouldBeGreyedOut)
    {
        this->itemIsGreyedOut = shouldBeGreyedOut;
        this->repaintItem();
    }
}

bool TreeItem::isGreyedOut() const noexcept
{
//    return !this->isSelected();
    return this->itemIsGreyedOut;
}

int TreeItem::getNumSelectedSiblings() const
{
    if (TreeItem *parent = dynamic_cast<TreeItem *>(this->getParentItem()))
    {
        return parent->getNumSelectedChildren();
    }

    return 0;
}

int TreeItem::getNumSelectedChildren() const
{
    int res = 0;

    for (int i = 0; i < this->getNumSubItems(); ++i)
    {
        res += (this->getSubItem(i)->isSelected() ? 1 : 0);
    }

    return res;
}

bool TreeItem::haveAllSiblingsSelected() const
{
    if (TreeItem *parent = dynamic_cast<TreeItem *>(this->getParentItem()))
    {
        if (parent->haveAllChildrenSelected())
        {
            return true;
        }
    }

    return false;
}

bool TreeItem::haveAllChildrenSelected() const
{
    for (int i = 0; i < this->getNumSubItems(); ++i)
    {
        if (! this->getSubItem(i)->isSelected())
        {
            return false;
        }
    }

    return true;
}

bool TreeItem::haveAllChildrenSelectedWithDeepSearch() const
{
    Array<TreeItem *> allChildren(this->findChildrenOfType<TreeItem>());

    for (int i = 0; i < allChildren.size(); ++i)
    {
        if (! allChildren.getUnchecked(i)->isSelected())
        {
            return false;
        }
    }

    return true;
}


//===----------------------------------------------------------------------===//
// Rename
//===----------------------------------------------------------------------===//

void TreeItem::onRename(const String &newName)
{
    //this->setSelected(true, true);
    this->safeRename(newName);
}

String TreeItem::getName() const noexcept
{
    return this->name;
}

void TreeItem::setName(const String &newName) noexcept
{
    this->name = newName;
}

String TreeItem::getCaption() const
{
    return this->getName();
}


//===----------------------------------------------------------------------===//
// Menu
//===----------------------------------------------------------------------===//

void TreeItem::itemSelectionChanged(bool isNowSelected)
{
    if (isNowSelected)
    {
        this->showPage();
    }
}

//===----------------------------------------------------------------------===//
// Cleanup
//===----------------------------------------------------------------------===//

bool TreeItem::deleteItem(TreeItem *itemToDelete)
{
    if (!itemToDelete) { return false; }

    if (itemToDelete->getRootTreeItem() != itemToDelete) // не удалять рут
    {
        WeakReference<TreeItem> switchTo = nullptr;
        WeakReference<TreeItem> parent = dynamic_cast<TreeItem *>(itemToDelete->getParentItem());

        const TreeItem *const markerItem = TreeItem::getActiveItem<TreeItem>(itemToDelete->getRootTreeItem());
        const bool markerItemIsDeleted = (markerItem == nullptr);

        if (itemToDelete->isMarkerVisible() || markerItemIsDeleted)
        {
            switchTo = parent;
        }

        delete itemToDelete;

        if (switchTo != nullptr)
        {
            switchTo->showPage();
        }
        else if (parent != nullptr)
        {
            while (parent)
            {
                if (parent->isMarkerVisible())
                {
                    parent->showPage();
                    break;
                }

                parent = dynamic_cast<TreeItem *>(parent->getParentItem());
            }
        }

        return true;
    }

    return false;
}

void TreeItem::deleteAllSelectedItems(Component *componentInTree)
{
    TreeItem *itemToDelete = TreeItem::getSelectedItem(componentInTree);
    TreeItem::deleteItem(itemToDelete);
}

void TreeItem::deleteAllSubItems()
{
    while (this->getNumSubItems() > 0)
    {
        delete this->getSubItem(0); // сам себя удалит из иерархии
    }
}

void TreeItem::removeItemFromParent()
{
    if (TreeItem *parent = dynamic_cast<TreeItem *>(this->getParentItem()))
    {
        for (int i = 0; i < parent->getNumSubItems(); ++i)
        {
            if (TreeItem *item = dynamic_cast<TreeItem *>(parent->getSubItem(i)))
            {
                if (item == this)
                {
                    parent->removeSubItem(i, false);
                }
            }
        }
    }
}


//===----------------------------------------------------------------------===//
// Painting
//===----------------------------------------------------------------------===//

Component *TreeItem::createItemComponent()
{
    if (! this->itemShouldBeVisible)
    {
        return nullptr;
    }

    if (this->isCompactMode())
    {
        return new TreeItemComponentCompact(*this);
    }
    
    return new TreeItemComponentDefault(*this);
}

void TreeItem::paintItem(Graphics &g, int width, int height)
{
    if (this->isCompactMode())
    {
        TreeItemComponentCompact::paintBackground(g, width, height, this->isSelected(), this->isMarkerVisible());
    }
    else
    {
        TreeItemComponentDefault::paintBackground(g, width, height, this->isSelected(), this->isMarkerVisible());
    }
}

void TreeItem::paintOpenCloseButton(Graphics &g, const Rectangle<float> &area,
                                    Colour backgroundColour, bool isMouseOver)
{
    //if (this->getNumSubItems() == 0) { return; }

//#if HELIO_MOBILE
//    return;
//#endif

    if (this->isCompactMode())
    { return; }

    Path p;
    p.addTriangle(0.0f, 0.0f, 1.0f, this->isOpen() ? 0.0f : 0.5f, this->isOpen() ? 0.5f : 0.0f, 1.0f);

    g.setColour(backgroundColour.contrasting().withAlpha(isMouseOver ? 0.7f : 0.4f));
    g.fillPath(p, p.getTransformToScaleToFit(area.reduced(1, area.getHeight() / 8), true));
}

void TreeItem::paintHorizontalConnectingLine(Graphics &g, const Line<float> &line)
{
//    if (this->isCompactMode())
//    {
//        TreeViewItem::paintHorizontalConnectingLine(g, line);
//    }
}

void TreeItem::paintVerticalConnectingLine(Graphics &g, const Line<float> &line)
{
//    if (this->isCompactMode())
//    {
//        TreeViewItem::paintVerticalConnectingLine(g, line);
//    }
}

int TreeItem::getItemHeight() const
{
    if (! this->itemShouldBeVisible)
    {
        return 0;
    }

    return this->isCompactMode() ? int(TREE_ITEM_HEIGHT * 1.2) : TREE_ITEM_HEIGHT;
}

Font TreeItem::getFont() const
{
    return Font(Font::getDefaultSansSerifFontName(), TREE_FONT_SIZE, Font::plain); //this->getItemHeight() * TREE_FONT_HEIGHT_PROPORTION, Font::plain);
}

Colour TreeItem::getColour() const
{
    return Colour(100, 150, 200);
    //return this->colour; // todo
}


// protected static

void TreeItem::openOrCloseAllSubGroups(TreeViewItem &item, bool shouldOpen)
{
    item.setOpen(shouldOpen);

    for (int i = item.getNumSubItems(); --i >= 0;)
    {
        if (TreeViewItem *sub = item.getSubItem(i))
        {
            openOrCloseAllSubGroups(*sub, shouldOpen);
        }
    }
}

void TreeItem::notifySubtreeMoved(TreeItem *node)
{
    node->onItemMoved();

    for (int i = 0; i < node->getNumSubItems(); ++i)
    {
        TreeItem *child = static_cast<TreeItem *>(node->getSubItem(i));
        TreeItem::notifySubtreeMoved(child);
    }
}

void TreeItem::getAllSelectedItems(Component *componentInTree, OwnedArray<TreeItem> &selectedNodes)
{
    TreeView *tree = dynamic_cast <TreeView *>(componentInTree);

    if (tree == nullptr)
    { tree = componentInTree->findParentComponentOfClass<TreeView>(); }

    if (tree != nullptr)
    {
        const int numSelected = tree->getNumSelectedItems();

        for (int i = 0; i < numSelected; ++i)
        {
            if (TreeItem *p = dynamic_cast <TreeItem *>(tree->getSelectedItem(i)))
            { selectedNodes.add(p); }
        }
    }
}

TreeItem *TreeItem::getSelectedItem(Component *componentInTree)
{
    TreeView *tree = dynamic_cast<TreeView *>(componentInTree);

    if (tree == nullptr)
    { tree = componentInTree->findParentComponentOfClass<TreeView>(); }

    if (tree != nullptr)
    {
        const int numSelected = tree->getNumSelectedItems();

        for (int i = 0; i < numSelected; ++i)
        {
            if (TreeItem *p = dynamic_cast<TreeItem *>(tree->getSelectedItem(i)))
            {
                return p;
            }
        }
    }

    return nullptr;
}

bool TreeItem::isNodeInChildren(TreeItem *nodeToScan, TreeItem *nodeToCheck)
{
    if (nodeToScan == nodeToCheck) { return true; }

    for (int i = 0; i < nodeToScan->getNumSubItems(); ++i)
    {
        TreeItem *child = static_cast<TreeItem *>(nodeToScan->getSubItem(i));

        if (child == nodeToCheck) { return true; }

        if (TreeItem::isNodeInChildren(child, nodeToCheck)) { return true; }
    }

    return false;
}


void TreeItem::itemDropped(const DragAndDropTarget::SourceDetails &dragSourceDetails, int insertIndex)
{
    if (TreeView *tree = dynamic_cast<TreeView *>(dragSourceDetails.sourceComponent.get()))
    {
        TreeItem *selected = TreeItem::getSelectedItem(tree);

        if (!selected) { return; }

        TreeViewItem *parent = selected->getParentItem();

        int insertIndexCorrection = 0;

        for (int i = 0; i < parent->getNumSubItems(); ++i)
        {
            if (TreeItem *item = dynamic_cast<TreeItem *>(parent->getSubItem(i)))
            {
                if (item == selected)
                {
                    // фикс для корявости в поведении TreeView при драге на себя же:
                    if ((parent == this) && ((insertIndex == i) || (insertIndex == (i + 1))))
                    { return; }

                    // после удаления индексы дерева поменяются
                    if ((parent == this) && (insertIndex > i))
                    { insertIndexCorrection = -1; }

                    parent->removeSubItem(i, false);
                }
            }
        }

        this->addChildTreeItem(selected, insertIndex + insertIndexCorrection);
    }
}

void TreeItem::setVisible(bool shouldBeVisible) noexcept
{
    this->itemShouldBeVisible = shouldBeVisible;
}

void TreeItem::safeRename(const String &newName)
{
    this->setName(TreeItem::createSafeName(newName));
}

bool TreeItem::isCompactMode() const
{
    if (this->getOwnerView() != nullptr)
    {
        return (this->getOwnerView()->getWidth() == TREE_COMPACT_WIDTH);
    }
    
    return false;
}
