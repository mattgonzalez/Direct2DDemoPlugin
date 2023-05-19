#include <JuceHeader.h>
#include "ValueTreeDisplay.h"

juce::String const leftBracket("[");
juce::String const rightBracket("] ");

static juce::String varToString(juce::var const &v, juce::String const &indent = {})
{
    if (v.isArray())
    {
        juce::String text(indent + "Array (length " + juce::String(v.size()) + ")");

        if (v.size() > 0)
        {
            text += juce::newLine;
            text += indent;
        }

        for (int arrayIndex = 0; arrayIndex < v.size(); ++arrayIndex)
        {
            juce::var const &entry(v[arrayIndex]);
            text += juce::String(arrayIndex) + ": ";
            text += varToString(entry, indent + "   ");
            text += juce::newLine;
            text += indent;
        }
        
        return text;
    }

    if (v.isObject())
    {
        //
        // Dynamic object?
        //
        if (juce::DynamicObject* object = v.getDynamicObject())
        {
            juce::NamedValueSet &valueSet(object->getProperties());
            
            juce::String text(indent + "Object (" + juce::String(valueSet.size()) + ")");
            
            if (valueSet.size() > 0)
            {
                text += juce::newLine;
                text += indent;
                
                for (int propertyIndex = 0; propertyIndex < valueSet.size(); ++propertyIndex)
                {
                    juce::var const &property(valueSet.getValueAt(propertyIndex));
                    
                    text += "Property " + juce::String(propertyIndex) + " - ";
                    text += valueSet.getName(propertyIndex).toString() + ": ";
                    text += varToString(property, indent + "   ");
                    text += juce::newLine;
                    text += indent;
                }
            }
            
            return text;
        }
        
        //
        // Reference counted object?
        //
        if (juce::ReferenceCountedObject* object = v.getObject())
        {
            return "Reference counted object 0x" + juce::String((juce::pointer_sized_int)object);
        }
        
        return "null object";
    }

    if (v.isVoid())
    {
        return indent + "(void)";
    }

    if (v.isBinaryData())
    {
        return indent + "(binary data)";
    }

    if (v.isUndefined())
    {
        return indent + "(undefined)";
    }

    if (v.isMethod())
    {
        return indent + "(method)";
    }
    
    if (v.isInt64())
    {
        return indent + "(int64)" + v.toString();
    }
    
    if (v.isInt())
    {
        return indent + "(int)" + v.toString();
    }
    
    if (v.isDouble())
    {
        return indent + "(double)" + v.toString();
    }
    
    if (v.isString())
    {
        return indent + "(string)" + v.toString();
    }

    return indent + v.toString();
}

class PropertyTreeComponent : public juce::TreeView, public juce::ValueTree::Listener
{
private:
	class VarItem : public juce::TreeViewItem
	{
	public:
		VarItem( juce::var const &value_) :
			value(value_)
		{
			if (value_.isArray())
			{
				for (int i = 0; i < value_.size(); ++i)
				{
					addSubItem(new ArrayEntryVarItem(i, value_[i]));
				}
			}
			else if (juce::DynamicObject* object = value_.getDynamicObject())
			{
				juce::NamedValueSet& objectProperties(object->getProperties());
				for (int i = 0; i < objectProperties.size(); ++i)
				{
					juce::Identifier objectProperty(objectProperties.getName(i));
					addSubItem(new NamedVarItem(objectProperty, objectProperties[objectProperty]));
				}
			}
		}

        ~VarItem() override = default;

		bool mightContainSubItems() override
		{
			if (value.isArray())
			{ 
				return value.size() > 0;
			}

			if (juce::DynamicObject* object = value.getDynamicObject())
			{
				return object->getProperties().size() > 0;
			}

			return false;
		}

		juce::var const &value;
	};

	class NamedVarItem : public VarItem
	{
	public:
		NamedVarItem(juce::Identifier const property_, juce::var const &value_) :
			VarItem(value_),
			property(property_)
		{
		}

        ~NamedVarItem() override = default;

		void paintItem(juce::Graphics& g, int width, int height) override
		{
            juce::String text(leftBracket + property.toString() + rightBracket);
            if (value.isArray())
            {
                text += "array " + juce::String(value.size());
            }
            else if (value.isObject())
            {
                text += "object";
            }
            else if (value.isInt() && property.toString().endsWith("Color"))
            {
                juce::Colour c((uint32_t)(int)value);
                g.setColour(c);
                g.fillRect(width - height, 0, height, height);
                text += c.toDisplayString(true);
            }
            else
            {
                text += varToString(value);
            }
            
            g.setColour(juce::Colours::black);
			g.drawText(text, 0, 0, width, height, juce::Justification::centredLeft, true);
		}

		juce::Identifier const property;
	};

	class ArrayEntryVarItem : public VarItem
	{
	public:
		ArrayEntryVarItem(int const arrayIndex_, juce::var const &value_) :
			VarItem(value_),
			arrayIndex(arrayIndex_)
		{
		}

        ~ArrayEntryVarItem() override = default;

		void paintItem(juce::Graphics& g, int width, int height) override
		{
			g.setColour(juce::Colours::black);
            juce::String text(leftBracket + juce::String(arrayIndex) + rightBracket);
            if (value.isArray())
            {
                text += "array " + juce::String(value.size());
            }
            else if (value.isObject())
            {
                text += "object";
            }
            else
            {
                text += varToString(value);
            }
			g.drawText(text, 0, 0, width, height, juce::Justification::centredLeft, true);
		}

		int const arrayIndex;
	};

	class RootItem : public juce::TreeViewItem
	{
	public:
		RootItem(juce::ValueTree &selectedTree_) :
			selectedTree(selectedTree_)
		{
		}

        ~RootItem() override = default;

		bool mightContainSubItems() override
		{
			return selectedTree.getNumProperties() > 0;
		}

		void paintItem(juce::Graphics& g, int width, int height) override
		{
			g.setColour(juce::Colours::black);
			g.drawText(selectedTree.getType().toString(), 0, 0, width, height, juce::Justification::centredLeft, true);
		}

		void update()
		{
			clearSubItems();

			for (int i = 0; i < selectedTree.getNumProperties(); ++i)
			{
				juce::Identifier property(selectedTree.getPropertyName(i));
				addSubItem(new NamedVarItem(property, selectedTree[property]));
			}
		}

		juce::ValueTree &selectedTree;
	} rootItem;

	juce::ValueTree &selectedTree;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PropertyTreeComponent)

	void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier&) override
	{
		if (treeWhosePropertyHasChanged == selectedTree)
		{
			update();
		}
	}
    
    void valueTreeChildAdded(juce::ValueTree& /*parentTree*/, juce::ValueTree& /*childWhichHasBeenAdded*/) override {}
    void valueTreeChildRemoved(juce::ValueTree& /*parentTree*/, juce::ValueTree& /*childWhichHasBeenRemoved*/, int /*indexFromWhichChildWasRemoved*/) override {}
    void valueTreeChildOrderChanged(juce::ValueTree& /*parentTreeWhoseChildrenHaveMoved*/, int /*oldIndex*/, int /*newIndex*/) override {}
    void valueTreeParentChanged(juce::ValueTree& /*treeWhoseParentHasChanged*/) override {}
    
	void valueTreeRedirected(juce::ValueTree&) override
	{
		update();
	}

	void update()
	{
		rootItem.update();
	}

public:
	PropertyTreeComponent(juce::ValueTree &selectedTree_) :
		rootItem(selectedTree_),
		selectedTree(selectedTree_)
	{
		setDefaultOpenness(true);
		setRootItem(&rootItem);
		selectedTree_.addListener(this);
		update();
	}
};


ValueTreeDisplay::ValueTreeDisplay(juce::ValueTree const &tree_) noexcept :
    tree(tree_),
	propertyEditor(new PropertyTreeComponent(selectedTree)),
	layoutResizer(&layout, 1, false),
    closedItemTypes
    {
    }
{
	layout.setItemLayout(0, -0.1, -0.9, -0.6);
	layout.setItemLayout(1, 5, 5, 5);
	layout.setItemLayout(2, -0.1, -0.9, -0.4);

	setSize(1000, 700);

	addAndMakeVisible(treeView);
	addAndMakeVisible(propertyEditor.get());
	addAndMakeVisible(layoutResizer);

	treeView.setDefaultOpenness(true);

    tree.addListener(this);
    
    triggerAsyncUpdate();
}

ValueTreeDisplay::~ValueTreeDisplay()
{
	treeView.setRootItem(nullptr);
}

void ValueTreeDisplay::paint(juce::Graphics &g)
{
	g.setColour(juce::Colours::lightgrey.withAlpha(0.5f));
	g.fillRect(propertyEditor->getBounds());
}

void ValueTreeDisplay::resized()
{
    juce::Component * comps[] = { &treeView, &layoutResizer, propertyEditor.get() };
    int y = 0;
	layout.layOutComponents(comps, 3, 0, y, getWidth(), getHeight() - y, true, true);
}

void ValueTreeDisplay::handleAsyncUpdate()
{
    buildTreeItems();
}

void ValueTreeDisplay::buildTreeItems()
{
	if (tree.isValid())
	{
        item = std::make_unique<Item>(tree, selectedTree, closedItemTypes);
		treeView.setRootItem(item.get());
		selectedTree = tree;
	}
	else
	{
		treeView.setRootItem(nullptr);
		selectedTree = {};
	}
}

void ValueTreeDisplay::valueTreeRedirected(juce::ValueTree&)
{
//    DBG("ValueTreeDisplay::valueTreeRedirected " << treeWhichHasBeenChanged.getType().toString());
    
    triggerAsyncUpdate();
}

ValueTreeDisplay::Item::Item(juce::ValueTree tree_, juce::ValueTree &selectedTree_, juce::Array<juce::Identifier> const &closedItemTypes_) :
	tree(tree_),
	selectedTree(selectedTree_),
    closedItemTypes(closedItemTypes_)
{
    updateSubItems();
    
    if (closedItemTypes.contains(tree_.getType()))
    {
        setOpenness(Openness::opennessClosed);
    }
    
	tree.addListener(this);
}

ValueTreeDisplay::Item::~Item()
{
	clearSubItems();
}

bool ValueTreeDisplay::Item::mightContainSubItems()
{
	return tree.getNumChildren() > 0;
}

void ValueTreeDisplay::Item::updateSubItems()
{
	clearSubItems();

	for (int i = 0; i < tree.getNumChildren(); ++i)
    {
		addSubItem(new Item(tree.getChild(i), selectedTree, closedItemTypes));
    }
}

void ValueTreeDisplay::Item::paintItem(juce::Graphics & g, int w, int h)
{
	juce::Font font;
	juce::Font smallFont(11.0);

	if (isSelected())
		g.fillAll(juce::Colours::white);

	juce::String typeName = tree.getType().toString();
	const float nameWidth = font.getStringWidthFloat(typeName);
	const int propertyX = juce::roundToInt(nameWidth);

	g.setColour(juce::Colours::black);
	g.setFont(font);

	g.drawText(tree.getType().toString(), 0, 0, w, h, juce::Justification::left, false);

	int propertyW = w - propertyX;
	if (propertyW > 0)
	{
		g.setColour(juce::Colours::blue);
		g.setFont(smallFont);
        juce::String propertySummary;

		for (int i = 0; i < tree.getNumProperties(); ++i)
		{
			const juce::Identifier name = tree.getPropertyName(i).toString();
            propertySummary += leftBracket + name.toString() + rightBracket;
            juce::var const &value(tree[name]);
            if (value.isArray())
            {
                propertySummary += "(array " + juce::String(value.size()) + ")";
            }
            else if (value.isObject())
            {
                propertySummary += "(object)";
            }
            else if (value.isInt() || value.isInt64() || value.isBool() || value.isString())
            {
                propertySummary += varToString(value);
            }
		}

		g.drawText(propertySummary, propertyX, 0, propertyW, h, juce::Justification::left, true);
	}
}

void ValueTreeDisplay::Item::itemSelectionChanged(bool isNowSelected)
{
	if (isNowSelected)
	{
		selectedTree = tree;
	}
}

void ValueTreeDisplay::Item::valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier & /* property */)
{
//    DBG("ValueTreeDisplay::Item:::valueTreePropertyChanged " << treeWhosePropertyHasChanged.getType().toString() << "[" << property.toString() << "]:" << treeWhosePropertyHasChanged[property].toString());
//
	if (tree != treeWhosePropertyHasChanged)
        return;

    tree.removeListener(this);
    repaintItem();
	tree.addListener(this);
}

ValueTreeDisplayWindow::ValueTreeDisplayWindow() noexcept :
	juce::DocumentWindow("ValueTree", juce::Colours::lightgrey, 0),
    tabs(juce::TabbedButtonBar::TabsAtLeft)
{
	setUsingNativeTitleBar(true);

	setContentNonOwned(&tabs, false);

    if (auto primaryDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        juce::Rectangle<int> userArea{ primaryDisplay->userArea };
        int w = 500;
        setBounds(userArea.getRight() - w - 20, userArea.getY() + 20, w, userArea.getHeight() - 40);
    }
	setVisible(true);
    
    tabs.getTabbedButtonBar().setColour(juce::TabbedButtonBar::ColourIds::tabTextColourId, juce::Colours::darkgrey);
    tabs.getTabbedButtonBar().setColour(juce::TabbedButtonBar::ColourIds::frontTextColourId, juce::Colours::black);

	setResizable(true, false);
}

void ValueTreeDisplayWindow::addTree(juce::ValueTree const &tree_)
{
    tabs.addTab(tree_.getType().toString(),
                juce::Colour(0xffeeeeee),
                new ValueTreeDisplay(tree_),
                true);
}

void ValueTreeDisplayWindow::closeButtonPressed()
{
}

