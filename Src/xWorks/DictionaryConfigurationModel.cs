﻿// Copyright (c) 2014 SIL International
// This software is licensed under the LGPL, version 2.1 or later
// (http://www.gnu.org/licenses/lgpl-2.1.html)

using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml;
using System.Xml.Serialization;
using SIL.CoreImpl;
using SIL.FieldWorks.FDO;
using SIL.Utils;

namespace SIL.FieldWorks.XWorks
{
	/// <summary>
	/// A selection of dictionary elements and options, for configuring a dictionary publication.
	/// </summary>
	[XmlRoot(ElementName = "DictionaryConfiguration")]
	public class DictionaryConfigurationModel
	{
		public const string FileExtension = ".fwdictconfig";
		public const string AllReversalIndexes = "All Reversal Indexes";

		/// <summary>
		/// Trees of dictionary elements
		/// </summary>
		[XmlElement(ElementName = "ConfigurationItem")]
		public List<ConfigurableDictionaryNode> Parts { get; set; }

		/// <summary>
		/// Trees of shared dictionary elements
		/// </summary>
		[XmlArray(ElementName = "SharedItems")]
		[XmlArrayItem("ConfigurationItem", typeof(ConfigurableDictionaryNode))]
		public List<ConfigurableDictionaryNode> SharedItems { get; set; }

		/// <summary>
		/// Name of this dictionary configuration. eg "Stem-based"
		/// </summary>
		[XmlAttribute(AttributeName = "name")]
		public string Label { get; set; }

		/// <summary>
		/// The version of the DictionaryConfigurationModel for use in data migration etc.
		/// </summary>
		[XmlAttribute(AttributeName = "version")]
		public int Version { get; set; }

		[XmlAttribute(AttributeName = "lastModified", DataType = "date")]
		public DateTime LastModified { get; set; }

		/// <summary>
		/// Publications for which this view applies. <seealso cref="AllPublications"/>
		/// </summary>
		[XmlArray(ElementName = "Publications")]
		[XmlArrayItem(ElementName = "Publication")]
		public List<string> Publications { get; set; }

		/// <summary>
		/// Whether all current and future publications should be used by this configuration.
		/// </summary>
		[XmlAttribute(AttributeName = "allPublications")]
		public bool AllPublications { get; set; }

		/// <summary>
		/// The writing system of the configuration.
		/// </summary>
		[XmlAttribute(AttributeName = "writingSystem")]
		public string WritingSystem { get; set; }

		/// <summary>
		/// File where data is stored
		/// </summary>
		[XmlIgnore]
		public string FilePath { get; set; }

		/// <summary></summary>
		public void Save()
		{
			LastModified = DateTime.Now;
			var serializer = new XmlSerializer(typeof(DictionaryConfigurationModel));
			var settings = new XmlWriterSettings { Indent = true };
			using(var writer = XmlWriter.Create(FilePath, settings))
			{
				serializer.Serialize(writer, this);
			}
		}

		/// <summary></summary>
		public void Load(FdoCache cache)
		{
			var serializer = new XmlSerializer(typeof(DictionaryConfigurationModel));
			using(var reader = XmlReader.Create(FilePath))
			{
				var model = (DictionaryConfigurationModel)serializer.Deserialize(reader);
				Label = model.Label;
				WritingSystem = model.WritingSystem;
				LastModified = model.LastModified;
				Version = model.Version;
				Parts = model.Parts;
				AllPublications = model.AllPublications;
				if (AllPublications)
					Publications = GetAllPublications(cache);
				else
					Publications = LoadPublicationsSafe(model, cache);
			}
			SpecifyParentsAndReferences(Parts);
			// Handle any changes to the custom field definitions.  (See https://jira.sil.org/browse/LT-16430.)
			// The "Merge" method handles both additions and deletions.
			DictionaryConfigurationController.MergeCustomFieldsIntoDictionaryModel(cache, this);
			// Handle changes to the lists of complex form types and variant types.
			MergeTypesIntoDictionaryModel(cache, this);
			// Handle any deleted styles.  (See https://jira.sil.org/browse/LT-16501.)
			EnsureValidStylesInModel(cache);
		}

		public static void MergeTypesIntoDictionaryModel(FdoCache cache, DictionaryConfigurationModel model)
		{
			var complexTypes = new Set<Guid>();
			foreach (var pos in cache.LangProject.LexDbOA.ComplexEntryTypesOA.ReallyReallyAllPossibilities)
				complexTypes.Add(pos.Guid);
			complexTypes.Add(Common.Controls.XmlViewsUtils.GetGuidForUnspecifiedComplexFormType());
			var variantTypes = new Set<Guid>();
			foreach (var pos in cache.LangProject.LexDbOA.VariantEntryTypesOA.ReallyReallyAllPossibilities)
				variantTypes.Add(pos.Guid);
			variantTypes.Add(Common.Controls.XmlViewsUtils.GetGuidForUnspecifiedVariantType());
			var referenceTypes = new Set<Guid>();
			if (cache.LangProject.LexDbOA.ReferencesOA != null)
			{
				foreach (var pos in cache.LangProject.LexDbOA.ReferencesOA.PossibilitiesOS)
					referenceTypes.Add(pos.Guid);
			}
			foreach (var part in model.Parts)
			{
				FixTypeListOnNode(part, complexTypes, variantTypes, referenceTypes);
			}
		}

		private static void FixTypeListOnNode(ConfigurableDictionaryNode node, Set<Guid> complexTypes, Set<Guid> variantTypes, Set<Guid> referenceTypes)
		{
			if (node.DictionaryNodeOptions is DictionaryNodeListOptions)
			{
				var listOptions = (DictionaryNodeListOptions)node.DictionaryNodeOptions;
				switch (listOptions.ListId)
				{
					case DictionaryNodeListOptions.ListIds.Complex:
					{
						FixOptionsAccordingToCurrentTypes(listOptions.Options, complexTypes);
						break;
					}
					case DictionaryNodeListOptions.ListIds.Variant:
					{
						FixOptionsAccordingToCurrentTypes(listOptions.Options, variantTypes);
						break;
					}
					case DictionaryNodeListOptions.ListIds.Entry:
					{
						FixOptionsAccordingToCurrentTypes(listOptions.Options, referenceTypes);
						break;
					}
					case DictionaryNodeListOptions.ListIds.Sense:
					{
						FixOptionsAccordingToCurrentTypes(listOptions.Options, referenceTypes);
						break;
					}
					case DictionaryNodeListOptions.ListIds.Minor:
					{
						var complexAndVariant = complexTypes.Union(variantTypes);
						FixOptionsAccordingToCurrentTypes(listOptions.Options, complexAndVariant);
						break;
					}
					default:
					break;
				}
			}
			//Recurse into child nodes and fix the type lists on them
			if (node.Children != null)
			{
				foreach (var child in node.Children)
					FixTypeListOnNode(child, complexTypes, variantTypes, referenceTypes);
			}
		}

		private static void FixOptionsAccordingToCurrentTypes(List<DictionaryNodeListOptions.DictionaryNodeOption> options, Set<Guid> possibilities)
		{
			var currentGuids = new Set<Guid>();
			foreach (var opt in options)
			{
				Guid guid;
				if (Guid.TryParse(opt.Id, out guid))	// can be empty string
					currentGuids.Add(guid);
			}
			// add types that do not exist already
			foreach (var type in possibilities)
			{
				if (!currentGuids.Contains(type))
					options.Add(new DictionaryNodeListOptions.DictionaryNodeOption { Id = type.ToString(), IsEnabled = true });
			}
			// remove options that no longer exist
			for (var i = options.Count - 1; i >= 0; --i)
			{
				Guid guid;
				if (Guid.TryParse(options[i].Id, out guid) && !possibilities.Contains(guid))
					options.RemoveAt(i);
			}
		}

		public void EnsureValidStylesInModel(FdoCache cache)
		{
			var styles = cache.LangProject.StylesOC.ToDictionary(style => style.Name);
			foreach (var part in Parts)
			{
				if (IsMainEntry(part) && string.IsNullOrEmpty(part.Style))
					part.Style = "Dictionary-Normal";
				EnsureValidStylesInConfigNodes(part, styles);
			}
		}

		private static void EnsureValidStylesInConfigNodes(ConfigurableDictionaryNode node, Dictionary<string, IStStyle> styles)
		{
			if (!string.IsNullOrEmpty(node.Style) && !styles.ContainsKey(node.Style))
				node.Style = null;
			if (node.DictionaryNodeOptions != null)
				EnsureValidStylesInNodeOptions(node.DictionaryNodeOptions, styles);
			if (node.Children != null)
			{
				foreach (var child in node.Children)
					EnsureValidStylesInConfigNodes(child, styles);
			}
		}

		private static void EnsureValidStylesInNodeOptions(DictionaryNodeOptions options, Dictionary<string, IStStyle> styles)
		{
			var senseOptions = options as DictionaryNodeSenseOptions;
			if (senseOptions == null)
				return;
			if (!string.IsNullOrEmpty(senseOptions.NumberStyle) && !styles.ContainsKey(senseOptions.NumberStyle))
				senseOptions.NumberStyle = null;
		}

		private List<string> LoadPublicationsSafe(DictionaryConfigurationModel model, FdoCache cache)
		{
			if (model == null || model.Publications == null)
				return new List<string>();

			return FilterRealPublications(model.Publications, cache);
		}

		public List<string> GetAllPublications(FdoCache cache)
		{
			return cache.LangProject.LexDbOA.PublicationTypesOA.PossibilitiesOS.Select(p => p.Name.BestAnalysisAlternative.Text).ToList();
		}

		private List<string> FilterRealPublications(List<string> modelPublications, FdoCache cache)
		{
			List<ICmPossibility> allPossibilities =
				cache.LangProject.LexDbOA.PublicationTypesOA.PossibilitiesOS.ToList();
			var allPossiblePublicationsInAllWs = new HashSet<string>();
			foreach (ICmPossibility possibility in allPossibilities)
				foreach (int ws in cache.ServiceLocator.WritingSystems.CurrentAnalysisWritingSystems.Handles())
					allPossiblePublicationsInAllWs.Add(possibility.Name.get_String(ws).Text);
			var realPublications = modelPublications.Where(allPossiblePublicationsInAllWs.Contains).ToList();

			return realPublications;
		}

		/// <summary>
		/// Default constructor for easier testing.
		/// </summary>
		internal DictionaryConfigurationModel() {}

		/// <summary>Loads a DictionaryConfigurationModel from the given path</summary>
		public DictionaryConfigurationModel(string path, FdoCache cache)
		{
			FilePath = path;
			Load(cache);
		}

		/// <summary>Returns a deep clone of this DCM. Caller is responsible to choose a unique FilePath</summary>
		public DictionaryConfigurationModel DeepClone()
		{
			var clone = new DictionaryConfigurationModel();

			// Copy everything over at first, importantly handling strings and primitives.
			var properties = typeof(DictionaryConfigurationModel).GetProperties();
			foreach (var property in properties.Where(prop => prop.CanWrite)) // Skip any read-only properties
			{
				var originalValue = property.GetValue(this, null);
				property.SetValue(clone, originalValue, null);
			}

			// Deep-clone Parts
			if (Parts != null)
			{
				clone.Parts = Parts.Select(node => node.DeepCloneUnderSameParent()).ToList();
			}

			// Deep-clone SharedItems
			if (SharedItems != null)
			{
				clone.SharedItems = SharedItems.Select(node => node.DeepCloneUnderSameParent()).ToList();
			}

			// Clone Publications
			if (Publications != null)
			{
				clone.Publications = new List<string>(Publications);
			}

			return clone;
		}

		/// <summary>
		/// Assign Parent properties to descendants of nodes
		/// </summary>
		internal void SpecifyParentsAndReferences(List<ConfigurableDictionaryNode> nodes)
		{
			if (nodes == null)
				throw new ArgumentNullException();

			foreach (var node in nodes)
			{
				if (node.Children == null) // TODO pH 2016.03: || !node.Children.Any())
				{
					if (string.IsNullOrEmpty(node.ReferenceItem))
						continue;
					node.ReferencedNode = SharedItems.FirstOrDefault(si =>
						si.Label == node.ReferenceItem && si.FieldDescription == node.FieldDescription && si.SubField == node.SubField);
					if (node.ReferencedNode == null)
						throw new KeyNotFoundException(string.Format("Could not find Referenced Node named {0} for field {1}.{2}",
							node.ReferenceItem, node.FieldDescription, node.SubField));
					if (node.ReferencedNode.Parent != null)
						continue;
					node.ReferencedNode.Parent = node;
					SpecifyParentsAndReferences(new List<ConfigurableDictionaryNode> { node.ReferencedNode }); // REVIEW pH 2016.03: specify here, or all SI's together?
				}
				else
				{
					// REVIEW (Jason) 2016.03: should we allow mixed families? Should Referenced Children eclipse Children instead of as-is? also in node.ChildrenOrReferencedChildren
					//if (!string.IsNullOrEmpty(node.ReferenceItem)) // TODO pH 2016.03: decide, and implement accordingly
					//	throw new ArgumentException("We presently do not allow biological parents to adopt");
					foreach (var child in node.Children)
						child.Parent = node;
					SpecifyParentsAndReferences(node.Children);
				}
			}
		}

		public override string ToString()
		{
			return Label;
		}

		/// <summary>If node is a HeadWord node.</summary>
		internal static bool IsHeadWord(ConfigurableDictionaryNode node)
		{
			if (node == null)
				throw new ArgumentNullException("node");
			return node.CSSClassNameOverride == "headword" || node.CSSClassNameOverride == "mainheadword";
		}

		/// <summary>If node is a Main Entry node that should not be duplicated or edited.</summary>
		internal static bool IsReadonlyMainEntry(ConfigurableDictionaryNode node)
		{
			return IsMainEntry(node) && node.DictionaryNodeOptions == null;
		}

		/// <summary>If node is a Main Entry node.</summary>
		internal static bool IsMainEntry(ConfigurableDictionaryNode node)
		{
			if (node == null)
				throw new ArgumentNullException("node");
			switch (node.CSSClassNameOverride)
			{
				case "entry":
				case "mainentrycomplex":
				case "reversalindexentry":
					return true;
				default:
					return false;
			}
		}
	}
}
