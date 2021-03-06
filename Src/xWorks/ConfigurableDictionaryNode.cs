﻿// Copyright (c) 2014-2016 SIL International
// This software is licensed under the LGPL, version 2.1 or later
// (http://www.gnu.org/licenses/lgpl-2.1.html)

using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml.Serialization;
using SIL.Utils;

namespace SIL.FieldWorks.XWorks
{
	/// <summary>
	/// This class allows configuring an element of dictionary data
	/// </summary>
	[XmlType (AnonymousType = false, TypeName = @"ConfigurationItem")]
	public class ConfigurableDictionaryNode
	{
		public override String ToString()
		{
			return DisplayLabel ?? FieldDescription + (SubField == null ? "" : "_" + SubField);
		}

		/// <summary>
		/// The string table is used to localize strings plucked from the XML.  (Other external
		/// processing makes the localizations available.)
		/// </summary>
		[XmlIgnore]
		public StringTable StringTable { get; set; }

		/// <summary>
		/// The non-editable portion of the label to display for this node
		/// </summary>
		[XmlAttribute(AttributeName = "name")]
		public string Label { get; set; }

		/// <summary>
		/// A suffix to distinguish between duplicate nodes
		/// </summary>
		[XmlAttribute(AttributeName = "nameSuffix")]
		public string LabelSuffix { get; set; }

		/// <summary>
		/// Combination of Label and LabelSuffix, if set.  This is localized if at all possible.
		/// </summary>
		[XmlIgnore]
		public string DisplayLabel
		{
			get
			{
				if (StringTable == null)
				{
					if (LabelSuffix == null)
						return Label;
					return string.Format("{0} ({1})", Label, LabelSuffix);
				}
				var localLabel = StringTable.LocalizeAttributeValue(Label);
				if (LabelSuffix == null)
					return localLabel;
				var localSuffix = StringTable.LocalizeAttributeValue(LabelSuffix);
				return string.Format("{0} ({1})", localLabel, localSuffix);
			}
		}

		/// <summary>
		/// Whether this element of dictionary data is shown as part of the dictionary.
		/// </summary>
		[XmlAttribute(AttributeName = "isEnabled")]
		public bool IsEnabled { get; set; }

		/// <summary>
		/// Whether this element of dictionary data was duplicated from another element of dictionary data.
		/// </summary>
		[XmlAttribute(AttributeName = "isDuplicate")]
		public bool IsDuplicate { get; set; } // REVIEW (Hasso) 2014.04: could we use get { return !string.IsNullOrEmpty(NameSuffix); }?

		/// <summary>ShouldSerialize[Attribute] is a magic method to prevent serializing the default value. May not work until Mono 3.3.0</summary>
		public bool ShouldSerializeIsDuplicate() { return IsDuplicate; }

		/// <summary>
		/// Whether this element of dictionary data represents a custom field.
		/// </summary>
		[XmlAttribute(AttributeName = "isCustomField")]
		public bool IsCustomField { get; set; }

		/// <summary>ShouldSerialize[Attribute] is a magic method to prevent serializing the default value. May not work until Mono 3.3.0</summary>
		public bool ShouldSerializeIsCustomField() { return IsCustomField; }

		/// <summary>
		/// The style to apply to the data configured by this node
		/// </summary>
		[XmlAttribute(AttributeName = "style")]
		public string Style { get; set; }

		public enum StyleTypes
		{
			[XmlEnum("default")]
			Default = 0,
			[XmlEnum("character")]
			Character,
			[XmlEnum("paragraph")]
			Paragraph
		}

		/// <summary>
		/// Whether the node's style selection should use character or paragraph styles. Allows specifying special cases like Minor Entry - Components (LT-15834).
		/// </summary>
		[XmlAttribute(AttributeName="styleType")]
		public StyleTypes StyleType { get; set; }

		/// <summary>
		/// ShouldSerialize[Attribute] is a magic method to prevent serialization of the default value.
		/// XMLSerializer looks for this method to determine whether to serialize each Element and Attribute.
		/// May not work in Mono until Mono 3.3.0.
		/// </summary>
		public bool ShouldSerializeStyleType()
		{
			return StyleType != StyleTypes.Default;
		}

		/// <summary>
		/// String to apply before the content configured by this node
		/// </summary>
		[XmlAttribute(AttributeName = "before")]
		public string Before { get; set; }

		/// <summary>
		/// String to apply between content items configured by this node (only applicable to lists)
		/// </summary>
		[XmlAttribute(AttributeName = "between")]
		public string Between { get; set; }

		/// <summary>
		/// String to apply after the content configured by this node
		/// </summary>
		[XmlAttribute(AttributeName = "after")]
		public string After { get; set; }

		/// <summary>
		/// Information about the field in the FieldWorks model that this node is configuring
		/// </summary>
		[XmlAttribute(AttributeName = "field")]
		public string FieldDescription { get; set; }

		/// <summary>
		/// Additional information directing this configuration to a sub field, or property of a field, in the FieldWorks model
		/// </summary>
		[XmlAttribute(AttributeName = "subField")]
		public string SubField { get; set; }

		/// <summary>
		/// Normally the FieldDescription in a ConfigurationNode will be directly used as the class name for
		/// the css and xhtml generated at that node. This field is used to provide alternative class names either to match
		/// historical exports, or for other strong reasons which should be documented where the override is defined.
		/// </summary>
		[XmlAttribute(AttributeName = "cssClassNameOverride")]
		public string CSSClassNameOverride { get; set; }

		/// <summary>
		/// Type specific configuration options for this configurable node;
		/// </summary>
		[XmlElement("WritingSystemOptions", typeof(DictionaryNodeWritingSystemOptions))]
		[XmlElement("ListTypeOptions", typeof(DictionaryNodeListOptions))]
		[XmlElement("ComplexFormOptions", typeof(DictionaryNodeComplexFormOptions))]
		[XmlElement("SenseOptions", typeof(DictionaryNodeSenseOptions))]
		[XmlElement("PictureOptions", typeof(DictionaryNodePictureOptions))]
		public DictionaryNodeOptions DictionaryNodeOptions { get; set; }

		/// <summary>
		/// Ordered list of nodes contained by this configurable node
		/// </summary>
		[XmlElement(ElementName = "ConfigurationItem")]
		public List<ConfigurableDictionaryNode> Children { get; set; }

		/// <summary>
		/// Parent of this node, or null if this is a top-level node.
		/// </summary>
		[XmlIgnore]
		public ConfigurableDictionaryNode Parent { get; internal set; }

		/// <summary>
		/// Reference to (Label of) a shared configuration node in SharedItems or null.
		/// </summary>
		[XmlElement("ReferenceItem")]
		public string ReferenceItem { get; set; }

		/// <summary>
		/// The actual node denoted by ReferenceItem; null if none
		/// </summary>
		internal ConfigurableDictionaryNode ReferencedNode { get; set; }

		/// <summary>
		/// Children of this node, if any; otherwise, children of the ReferenceItem, if any
		/// </summary>
		[XmlIgnore]
		public List<ConfigurableDictionaryNode> ReferencedOrDirectChildren
		{ // TODO pH better name? Dependents? AllChildren? Niblets?
			get { return ReferencedNode == null ? Children : ReferencedNode.Children; }
		}

		/// <summary>
		/// Clone this node. Point to the same Parent object. Deep-clone Children and DictionaryNodeOptions.
		/// </summary>
		internal ConfigurableDictionaryNode DeepCloneUnderSameParent()
		{
			var clone = new ConfigurableDictionaryNode();

			// Copy everything over at first, importantly handling strings, bools, and Parent.
			var properties = typeof (ConfigurableDictionaryNode).GetProperties();
			foreach (var property in properties)
			{
				// Skip read-only properties (eg DisplayLabel)
				if (!property.CanWrite)
					continue;
				var originalValue = property.GetValue(this, null);
				property.SetValue(clone, originalValue, null);
			}

			// Deep-clone Children
			if (Children != null)
			{
				var clonedChildren = new List<ConfigurableDictionaryNode>();
				foreach (var child in Children)
				{
					var clonedChild = child.DeepCloneUnderSameParent();
					// Cloned children should point to their newly-cloned parent
					clonedChild.Parent = clone;
					clonedChildren.Add(clonedChild);
				}
				clone.Children = clonedChildren;
			}

			// Deep-clone DictionaryNodeOptions
			if (DictionaryNodeOptions != null)
				clone.DictionaryNodeOptions = DictionaryNodeOptions.DeepClone();

			return clone;
		}

		public override int GetHashCode()
		{
			return Parent == null ? DisplayLabel.GetHashCode() : DisplayLabel.GetHashCode() ^ Parent.GetHashCode();
		}

		public override bool Equals(object other)
		{
			var otherNode = other as ConfigurableDictionaryNode;
			if (otherNode == null || Label != otherNode.Label || LabelSuffix != otherNode.LabelSuffix || FieldDescription != otherNode.FieldDescription)
				return false;
			// The rules for our tree prevent two same-named nodes under a parent
			return CheckParents(this, otherNode);
		}

		/// <summary>
		/// A match is two nodes with the same label and suffix in the same hierarchy (all ancestors have same labels & suffixes)
		/// </summary>
		private static bool CheckParents(ConfigurableDictionaryNode first, ConfigurableDictionaryNode second)
		{
			if(first == null && second == null)
			{
				return true;
			}
			if((first == null ^ second == null) || (first.Parent == null ^ second.Parent == null)) // ^ is XOR
			{
				return false;
			}
			return first.Label == second.Label && first.LabelSuffix == second.LabelSuffix && CheckParents(first.Parent, second.Parent);
		}

		/// <summary>
		/// Duplicate this node and its Children, adding the result to the Parent's list of children.
		/// </summary>
		public ConfigurableDictionaryNode DuplicateAmongSiblings()
		{
			return DuplicateAmongSiblings(Parent.Children);
		}

		/// <summary>
		/// Duplicate this node and its Children, adding the result among the list of siblings.
		/// </summary>
		public ConfigurableDictionaryNode DuplicateAmongSiblings(List<ConfigurableDictionaryNode> siblings)
		{
			var duplicate = DeepCloneUnderSameParent();
			duplicate.IsDuplicate = true;

			// Provide a suffix to distinguish among similar dictionary items.
			int suffix = 1;
			while (siblings.Exists(sibling => sibling.Label == this.Label && sibling.LabelSuffix == suffix.ToString()))
			{
				suffix++;
			}
			duplicate.LabelSuffix = suffix.ToString();

			var locationOfThisNode = siblings.IndexOf(this);
			siblings.Insert(locationOfThisNode + 1, duplicate);
			return duplicate;
		}

		/// <summary>
		/// Disassociate this node from its current Parent.
		/// </summary>
		public void UnlinkFromParent()
		{
			if (Parent == null)
				return;

			Parent.Children.Remove(this);
			Parent = null;
		}

		/// <summary>
		/// Change suffix. Must be unique among sibling dictionary items with the same label. It's okay to request to change to the current suffix.
		/// </summary>
		public bool ChangeSuffix(string newSuffix)
		{
			return ChangeSuffix(newSuffix, Parent.Children);
		}

		public bool ChangeSuffix(string newSuffix, List<ConfigurableDictionaryNode> siblings)
		{
			if (siblings.Exists(sibling => !ReferenceEquals(sibling, this) && sibling.Label == this.Label && sibling.LabelSuffix == newSuffix))
				return false;

			LabelSuffix = newSuffix;
			return true;
		}
	}
}
