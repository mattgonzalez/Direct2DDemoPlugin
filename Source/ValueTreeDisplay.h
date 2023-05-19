#pragma once

class ValueTreeDisplay :
    public juce::Component,
    public juce::ValueTree::Listener,
    public juce::AsyncUpdater
{
public:
	ValueTreeDisplay(juce::ValueTree const &tree_) noexcept;
	~ValueTreeDisplay() override;

	void paint(juce::Graphics &g) override;
	void resized() override;

    void valueTreePropertyChanged(juce::ValueTree& /*treeWhosePropertyHasChanged*/, const juce::Identifier& /*property*/) override {}
    void valueTreeChildAdded(juce::ValueTree& /*parentTree*/, juce::ValueTree& /*childWhichHasBeenAdded*/) override {}
    void valueTreeChildRemoved(juce::ValueTree& /*parentTree*/, juce::ValueTree& /*childWhichHasBeenRemoved*/, int /*indexFromWhichChildWasRemoved*/) override {}
    void valueTreeChildOrderChanged(juce::ValueTree& /*parentTreeWhoseChildrenHaveMoved*/, int /*oldIndex*/, int /*newIndex*/) override {}
    void valueTreeParentChanged(juce::ValueTree& /*treeWhoseParentHasChanged*/) override {}
    void valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged) override;
    
    void handleAsyncUpdate() override;

private:
	//
	// Item is a custom juce::TreeViewItem.
	//
	class Item : public juce::TreeViewItem, public juce::ValueTree::Listener
	{
	public:
		Item(juce::ValueTree tree_, juce::ValueTree &selectedTree_, juce::Array<juce::Identifier> const &closedItemTypes_);
		~Item() override;

		bool mightContainSubItems() override;

		void paintItem(juce::Graphics & g, int w, int h) override;

		void itemSelectionChanged(bool isNowSelected) override;

		void valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property) override;

		void valueTreeChildAdded(juce::ValueTree &/*parentTree*/, juce::ValueTree &/*childWhichHasBeenAdded*/) override
		{
			updateSubItems();
		}

		void valueTreeChildRemoved(juce::ValueTree &/*parentTree*/, juce::ValueTree &/*childWhichHasBeenRemoved*/, int) override
		{
			updateSubItems();
		}

		void valueTreeChildOrderChanged(juce::ValueTree &/*parentTreeWhoseChildrenHaveMoved*/, int, int) override
		{
			updateSubItems();
		}

		void valueTreeParentChanged(juce::ValueTree &/*treeWhoseParentHasChanged*/) override
		{
			updateSubItems();
		}

		void valueTreeRedirected(juce::ValueTree &/*treeWhichHasBeenChanged*/) override
		{
			updateSubItems();
		}

	private:
		juce::ValueTree tree;
		juce::ValueTree &selectedTree;
		juce::Array<juce::Identifier> currentProperties;
        juce::Array<juce::Identifier> const &closedItemTypes;

		void updateSubItems();

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Item)
	};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ValueTreeDisplay)
    juce::ValueTree tree;
	juce::ValueTree selectedTree;
	std::unique_ptr<Item> item;
	juce::TreeView treeView;
	std::unique_ptr<juce::Component> propertyEditor;
    juce::StretchableLayoutManager layout;
    juce::StretchableLayoutResizerBar layoutResizer;
    juce::Array<juce::Identifier> const closedItemTypes;

	void buildTreeItems();
};

class ValueTreeDisplayWindow : public juce::DocumentWindow
{
public:
	ValueTreeDisplayWindow() noexcept;
    
    void addTree(juce::ValueTree const &tree_);

    void closeButtonPressed() override;

private:
    juce::TabbedComponent tabs;
    
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ValueTreeDisplayWindow)
};
