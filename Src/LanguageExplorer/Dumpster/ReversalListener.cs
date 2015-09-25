// Copyright (c) 2015 SIL International
// This software is licensed under the LGPL, version 2.1 or later
// (http://www.gnu.org/licenses/lgpl-2.1.html)

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Xml;
using System.Windows.Forms;
using LanguageExplorer.Areas.Lexicon;
using LanguageExplorer.Areas.Lexicon.Tools.ReversalIndexes;
using SIL.CoreImpl;
using SIL.FieldWorks.Common.Controls;
using SIL.FieldWorks.Common.Framework;
using SIL.FieldWorks.Common.FwUtils;
using SIL.FieldWorks.Common.RootSites;
using SIL.FieldWorks.FDO;
using SIL.FieldWorks.FDO.Infrastructure;
using SIL.FieldWorks.FdoUi.Dialogs;
using SIL.FieldWorks.Filters;
using SIL.FieldWorks.FdoUi;
using SIL.FieldWorks.FDO.Application;
using SIL.FieldWorks.XWorks;
using SIL.Utils;

namespace LanguageExplorer.Dumpster
{
#if RANDYTODO
	// TODO: I don't expect this class to survive, but its useful code moved elsewhere, as ordinary event handlers.
/*
<listener assemblyPath="LanguageExplorer.dll" class="LanguageExplorer.Dumpster.ReversalListener">
	<parameters clerk="AllReversalEntries"/>
</listener>

<clerk id="AllReversalEntries">
	<dynamicloaderinfo assemblyPath="LanguageExplorer.dll" class="LanguageExplorer.Dumpster.ReversalEntryClerk"/>
	<recordList owner="ReversalIndex" property="AllEntries">
		<dynamicloaderinfo assemblyPath="LanguageExplorer.dll" class="LanguageExplorer.Areas.Lexicon.AllReversalEntriesRecordList"/>
	</recordList>
	<filters/>
	<sortMethods>
		<sortMethod label="Form" assemblyPath="Filters.dll" class="SIL.FieldWorks.Filters.PropertyRecordSorter" sortProperty="ShortName"/>
	</sortMethods>
</clerk>
*/
#endif
	/// <summary>
	/// A listener class for reversal issues.
	/// This class currently handles these issues:
	/// 1. 'Find' dlg for reversal entries.
	/// 2.
	/// </summary>
	internal sealed class ReversalListener : IFlexComponent, IFWDisposable
	{
		#region IDisposable & Co. implementation
		// Region last reviewed: never

		/// <summary>
		/// Check to see if the object has been disposed.
		/// All public Properties and Methods should call this
		/// before doing anything else.
		/// </summary>
		public void CheckDisposed()
		{
			if (IsDisposed)
				throw new ObjectDisposedException(String.Format("'{0}' in use after being disposed.", GetType().Name));
		}

		/// <summary>
		/// True, if the object has been disposed.
		/// </summary>
		private bool m_isDisposed;

		private int instanceID = 0x00000F0;

		/// <summary>
		/// See if the object has been disposed.
		/// </summary>
		public bool IsDisposed
		{
			get { return m_isDisposed; }
		}

		/// <summary>
		/// Finalizer, in case client doesn't dispose it.
		/// Force Dispose(false) if not already called (i.e. m_isDisposed is true)
		/// </summary>
		/// <remarks>
		/// In case some clients forget to dispose it directly.
		/// </remarks>
		~ReversalListener()
		{
			Dispose(false);
			// The base class finalizer is called automatically.
		}

		/// <summary>
		///
		/// </summary>
		/// <remarks>Must not be virtual.</remarks>
		public void Dispose()
		{
			Dispose(true);
			// This object will be cleaned up by the Dispose method.
			// Therefore, you should call GC.SupressFinalize to
			// take this object off the finalization queue
			// and prevent finalization code for this object
			// from executing a second time.
			GC.SuppressFinalize(this);
		}

		/// <summary>
		/// Executes in two distinct scenarios.
		///
		/// 1. If disposing is true, the method has been called directly
		/// or indirectly by a user's code via the Dispose method.
		/// Both managed and unmanaged resources can be disposed.
		///
		/// 2. If disposing is false, the method has been called by the
		/// runtime from inside the finalizer and you should not reference (access)
		/// other managed objects, as they already have been garbage collected.
		/// Only unmanaged resources can be disposed.
		/// </summary>
		/// <param name="disposing"></param>
		/// <remarks>
		/// If any exceptions are thrown, that is fine.
		/// If the method is being done in a finalizer, it will be ignored.
		/// If it is thrown by client code calling Dispose,
		/// it needs to be handled by fixing the bug.
		///
		/// If subclasses override this method, they should call the base implementation.
		/// </remarks>
		private void Dispose(bool disposing)
		{
			System.Diagnostics.Debug.WriteLineIf(!disposing, "****** Missing Dispose() call for " + GetType().Name + ". ****** ");
			// Must not be run more than once.
			if (m_isDisposed)
				return;

			if (disposing)
			{
				// Dispose managed resources here.
			}

			// Dispose unmanaged resources here, whether disposing is true or false.
			PropertyTable = null;
			Publisher = null;
			Subscriber = null;

			m_isDisposed = true;
		}

		#endregion IDisposable & Co. implementation

		#region Implementation of IPropertyTableProvider

		/// <summary>
		/// Placement in the IPropertyTableProvider interface lets FwApp call IPropertyTable.DoStuff.
		/// </summary>
		public IPropertyTable PropertyTable { get; private set; }

		#endregion

		#region Implementation of IPublisherProvider

		/// <summary>
		/// Get the IPublisher.
		/// </summary>
		public IPublisher Publisher { get; private set; }

		#endregion

		#region Implementation of ISubscriberProvider

		/// <summary>
		/// Get the ISubscriber.
		/// </summary>
		public ISubscriber Subscriber { get; private set; }

		/// <summary>
		/// Initialize a FLEx component with the basic interfaces.
		/// </summary>
		/// <param name="propertyTable">Interface to a property table.</param>
		/// <param name="publisher">Interface to the publisher.</param>
		/// <param name="subscriber">Interface to the subscriber.</param>
		public void InitializeFlexComponent(IPropertyTable propertyTable, IPublisher publisher, ISubscriber subscriber)
		{
			FlexComponentCheckingService.CheckInitializationValues(propertyTable, publisher, subscriber, PropertyTable, Publisher, Subscriber);

			PropertyTable = propertyTable;
			Publisher = publisher;
			Subscriber = subscriber;

			var cache = PropertyTable.GetValue<FdoCache>("cache");
			cache.DomainDataByFlid.BeginNonUndoableTask();
			var usedWses = new List<IWritingSystem>();
			foreach (IReversalIndex rev in cache.LanguageProject.LexDbOA.ReversalIndexesOC)
			{
				var ws = cache.ServiceLocator.WritingSystemManager.get_Engine(rev.WritingSystem);
				usedWses.Add((IWritingSystem)ws);
				if (rev.PartsOfSpeechOA == null)
					rev.PartsOfSpeechOA = cache.ServiceLocator.GetInstance<ICmPossibilityListFactory>().Create();
				rev.PartsOfSpeechOA.ItemClsid = PartOfSpeechTags.kClassId;
			}
			List<IReversalIndex> corruptReversalIndices = new List<IReversalIndex>();
			foreach (IReversalIndex rev in cache.LanguageProject.LexDbOA.ReversalIndexesOC)
			{
				// Make sure each index has a name, if it is available from the writing system.
				if (string.IsNullOrEmpty(rev.WritingSystem))
				{
					// Delete a bogus IReversalIndex that has no writing system.
					// But, for now only store them for later deletion,
					// as immediate removal will wreck the looping.
					corruptReversalIndices.Add(rev);
					continue;
				}
				IWritingSystem revWs = cache.ServiceLocator.WritingSystemManager.Get(rev.WritingSystem);
				// TODO WS: is DisplayLabel the right thing to use here?
				rev.Name.SetAnalysisDefaultWritingSystem(revWs.DisplayLabel);
			}
			// Delete any corrupt reversal indices.
			foreach (IReversalIndex rev in corruptReversalIndices)
			{
				MessageBox.Show("Need to delete a corrupt reversal index (no writing system)", "Self-correction");
				cache.LangProject.LexDbOA.ReversalIndexesOC.Remove(rev);	// does this accomplish anything?
			}

			// Set up for the reversal index combo box or dropdown menu.
			Guid firstGuid = Guid.Empty;
			List<IReversalIndex> reversalIds = cache.LanguageProject.LexDbOA.CurrentReversalIndices;
			if (reversalIds.Count > 0)
				firstGuid = reversalIds[0].Guid;
			else if (cache.LanguageProject.LexDbOA.ReversalIndexesOC.Count > 0)
				firstGuid = cache.LanguageProject.LexDbOA.ReversalIndexesOC.ToGuidArray()[0];
			if (firstGuid != Guid.Empty)
			{
				SetReversalIndexGuid(firstGuid);
			}
			cache.DomainDataByFlid.EndNonUndoableTask();
		}

		#endregion

		private void SetReversalIndexGuid(Guid reversalIndexGuid)
		{
			PropertyTable.SetProperty("ReversalIndexGuid", reversalIndexGuid.ToString(), true, true);
		}

		#region XCore Message handlers

		#region Go Dlg
		/// <summary>
		/// Handles the xCore message to go to a reversal entry.
		/// </summary>
		/// <param name="argument">The xCore Command object.</param>
		/// <returns>true</returns>
		public bool OnGotoReversalEntry(object argument)
		{
			CheckDisposed();
			using (var dlg = new ReversalEntryGoDlg())
			{
				dlg.ReversalIndex = Entry.ReversalIndex;
				var cache = PropertyTable.GetValue<FdoCache>("cache");
				dlg.SetDlgInfo(cache, null, PropertyTable, Publisher);
				if (dlg.ShowDialog() == DialogResult.OK)
				{
					// Can't Go to a subentry, so we have to go to its main entry.
					var selEntry = (IReversalIndexEntry) dlg.SelectedObject;
					Publisher.Publish("JumpToRecord", selEntry.MainEntry.Hvo);
				}
			}
			return true;
		}

		private IReversalIndexEntry Entry
		{
			get
			{
				IReversalIndexEntry rie = null;
#if RANDYTODO
				// TODO: Use another mechanism to get the clerk in whatever replaces this listener.
				// TODO: It will be an instance of "ReversalEntryClerk" (see above for details)
				string clerkId = XmlUtils.GetManditoryAttributeValue(m_configurationParameters, "clerk");
				string propertyName = RecordClerk.GetCorrespondingPropertyName(clerkId);
				RecordClerk clerk = PropertyTable.GetValue<RecordClerk>(propertyName);
				if (clerk != null)
					rie = clerk.CurrentObject as IReversalIndexEntry;
#endif
				return rie;
			}
		}

#if RANDYTODO
		public virtual bool OnDisplayGotoReversalEntry(object commandObject,
			ref UIItemDisplayProperties display)
		{
			CheckDisposed();

			IReversalIndexEntry rie = Entry;
			if (rie == null || rie.Owner.Hvo == 0)
			{
				display.Enabled = display.Visible = false;
			}
			else
			{
				display.Enabled = rie.ReversalIndex.EntriesOC.Count > 1 && InFriendlyArea;
				display.Visible = InFriendlyArea;
			}
			return true; //we've handled this
		}
#endif
		#endregion Go Dlg

		#region Reversal Index Combo

#if RANDYTODO
		/// <summary>
		/// Called (by xcore) to control display params of the reversal index menu, e.g. whether it should be enabled.
		/// </summary>
		/// <param name="commandObject"></param>
		/// <param name="display"></param>
		/// <returns></returns>
		public virtual bool OnDisplayReversalIndexHvo(object commandObject, ref UIItemDisplayProperties display)
		{
			CheckDisposed();

			// Do NOT check InFriendlyArea. This menu should be enabled in every context where it occurs at all.
			// And, it gets tested during creation of the pane bar, BEFORE the properties InFriendlyArea uses
			// are set, so we get inaccurate answers.
			display.Enabled = true; // InFriendlyArea;
			display.Visible = display.Enabled;

			return true; // We dealt with it.
		}

		/// <summary>
		/// This is called when XCore wants to display something that relies on the list with the
		/// id "ReversalIndexList"
		/// </summary>
		/// <param name="parameter"></param>
		/// <param name="display"></param>
		/// <returns></returns>
		public bool OnDisplayReversalIndexList(object parameter, ref UIListDisplayProperties display)
		{
			CheckDisposed();

			display.List.Clear();
			var cache = m_propertyTable.GetValue<FdoCache>("cache");
			// List all existing reversal indexes.  (LT-4479, as amended)
			//IReversalIndex riOwner = this.IReversalIndex;
			foreach (IReversalIndex ri in cache.LanguageProject.LexDbOA.ReversalIndexesOC)
			{
				display.List.Add(ri.ShortName, ri.Guid.ToString(), null, null);
			}
			display.List.Sort();
			return true; // We handled this, no need to ask anyone else.
		}
#endif

		/// <summary />
		public bool OnInsertReversalIndex_FORCE(object argument)
		{
			CheckDisposed();

			Guid newGuid = CreateNewReversalIndex(false);
			if (newGuid != Guid.Empty)
			{
				SetReversalIndexGuid(newGuid);
			}
			return true;
		}

		private Guid CreateNewReversalIndex(bool allowCancel)
		{
			using (var dlg = new CreateReversalIndexDlg())
			{
				var cache = PropertyTable.GetValue<FdoCache>("cache");
				dlg.Init(cache, allowCancel);
				// Don't bother if all languages already have a reversal index!
				if (dlg.PossibilityCount > 0)
				{
					if (dlg.ShowDialog() == DialogResult.OK)
					{
						int hvo = dlg.NewReversalIndexHvo;
						return cache.ServiceLocator.GetInstance<ICmObjectRepository>().GetObject(hvo).Guid;
					}
				}
			}
			return Guid.Empty;
		}

		#endregion Reversal Index Combo

		/// <summary>
		///
		/// </summary>
		/// <remarks>
		/// This is something of a hack until we come up with a generic solution to
		/// the problem on how to control we are CommandSet are handled by listeners are
		/// visible.
		/// </remarks>
		private bool InFriendlyArea
		{
			get
			{
				string areaChoice = PropertyTable.GetValue<string>("areaChoice");
				string toolFor = PropertyTable.GetValue<string>("ToolForAreaNamed_lexicon");

				return areaChoice == "lexicon" && toolFor.StartsWith("reversalTool");
			}
		}

		#endregion XCore Message handlers
	}

	/// <summary>
	/// This clerk is used to deal with POSes/Entries of a IReversalIndex.
	/// It's subclasses do the object-specific kinds of work.
	/// </summary>
	internal abstract class ReversalClerk : RecordClerk
	{
		/// <summary>
		/// Contructor.
		/// </summary>
		/// <param name="id">Clerk id/name.</param>
		/// <param name="recordList">Record list for the clerk.</param>
		/// <param name="defaultSorter">The default record sorter.</param>
		/// <param name="defaultSortLabel"></param>
		/// <param name="defaultFilter">The default filter to use.</param>
		/// <param name="allowDeletions"></param>
		/// <param name="shouldHandleDeletion"></param>
		internal ReversalClerk(string id, RecordList recordList, RecordSorter defaultSorter, string defaultSortLabel, RecordFilter defaultFilter, bool allowDeletions, bool shouldHandleDeletion)
			: base(id, recordList, defaultSorter, defaultSortLabel, defaultFilter, allowDeletions, shouldHandleDeletion)
		{
		}

		#region Overrides of RecordClerk

		/// <summary>
		/// Initialize a FLEx component with the basic interfaces.
		/// </summary>
		/// <param name="propertyTable">Interface to a property table.</param>
		/// <param name="publisher">Interface to the publisher.</param>
		/// <param name="subscriber">Interface to the subscriber.</param>
		public override void InitializeFlexComponent(IPropertyTable propertyTable, IPublisher publisher, ISubscriber subscriber)
		{
			base.InitializeFlexComponent(propertyTable, publisher, subscriber);

			ChangeOwningObjectIfPossible();
		}

		#endregion

		private void ChangeOwningObjectIfPossible()
		{
			var newGuid = ReversalIndexEntryUi.GetObjectGuidIfValid(PropertyTable, "ReversalIndexGuid");
			try
			{
				ChangeOwningObject(newGuid);
			}
			catch
			{
				// Can't change an owner if we have a bad guid.
			}
		}

		private void ChangeOwningObject(Guid newGuid)
		{
			if (newGuid.Equals(Guid.Empty))
			{
				// We need to find another reversal index. Any will do.
				newGuid = Cache.ServiceLocator.GetInstance<IReversalIndexRepository>().AllInstances().First().Guid;
				PropertyTable.SetProperty("ReversalIndexGuid", newGuid.ToString(), true, true);
			}

			var ri = Cache.ServiceLocator.GetObject(newGuid) as IReversalIndex;
			if (ri == null)
			{
				return;
			}

			var layoutName = String.Format("publishReversal-{0}", ri.WritingSystem);
			PropertyTable.SetProperty("ReversalIndexPublicationLayout", layoutName, true, true);

			ICmObject newOwningObj = NewOwningObject(ri);
			if (newOwningObj != OwningObject)
			{
				UpdateFiltersAndSortersIfNeeded(); // Load the index-specific sorter
				OnChangeSorter(); // Update the column headers with sort arrows
				OwningObject = newOwningObj; // This automatically reloads (and sorts) the list
				PropertyTable.SetProperty("ActiveClerkOwningObject", newOwningObj, false, true);
				Publisher.Publish("ClerkOwningObjChanged", this);
			}
		}

		/// <summary>
		/// returns the XmlNode which configures the FormColumn in the BrowseView associated with BulkEdit of ReversalEntries
		/// </summary>
		protected XmlNode BrowseViewFormCol
		{
			get
			{
#if RANDYTODO
				// TODO: This just won't work in the new world order!
#endif
				var path = Path.Combine(Path.Combine(Path.Combine(Path.Combine(FwDirectoryFinder.CodeDirectory,
																			   FwDirectoryFinder.ksFlexFolderName),
																  @"Configuration"),
													 @"Lexicon"),
										@"ReversalEntriesBulkEdit");
				var doc = new XmlDocument();
				doc.Load(Path.Combine(path, @"toolConfiguration.xml"));
				var columnNode = doc.SelectSingleNode(@"//column[@label='Form']");
				return columnNode;
			}
		}

		/// <summary>
		/// The reversal should not be checking the writing system when testing for sorter compatibility since
		/// that writing system is changed in the Clerk through events and the bulkedit and browse view share the same clerk.
		/// </summary>
		/// <param name="first"></param>
		/// <param name="second"></param>
		/// <remarks>This method is only valid because there are no multi-lingual columns in the reversal views</remarks>
		/// <returns></returns>
		public override bool AreSortersCompatible(RecordSorter first, RecordSorter second)
		{
			if (first == null || second == null)
				return false;

			var secondSorter = second as GenRecordSorter;
			var firstSorter = first as GenRecordSorter;
			if (secondSorter == null || firstSorter == null)
				return first.CompatibleSorter(second);

			var sfcThis = firstSorter.Comparer as StringFinderCompare;
			var sfcOther = secondSorter.Comparer as StringFinderCompare;
			if (sfcThis == null || sfcOther == null)
				return false;
			if (!sfcThis.Finder.SameFinder(sfcOther.Finder))
				return false;
			return true;
		}

		/// <summary>
		/// The stored sorter files keep messing us up here, so we need to do a bit of post-deserialization processing.
		/// </summary>
		/// <returns>true if we restored something different from what was already there.</returns>
		protected override bool TryRestoreSorter()
		{
			var fakevc = new XmlBrowseViewBaseVc
			{
				SuppressPictures = true, // SuppressPictures to make sure that we don't leak anything as this will not be disposed.
				Cache = Cache
			};
			if (base.TryRestoreSorter() && Sorter is GenRecordSorter)
			{
				var sorter = (GenRecordSorter)Sorter;
				var stringFinderComparer = sorter.Comparer as StringFinderCompare;
				if (stringFinderComparer != null)
				{
					var colSpec = ReflectionHelper.GetField(stringFinderComparer.Finder, "m_colSpec") as XmlNode ?? BrowseViewFormCol;
					sorter.Comparer = new StringFinderCompare(LayoutFinder.CreateFinder(Cache, colSpec, fakevc,
																						PropertyTable.GetValue<IApp>("App")),
															stringFinderComparer.SubComparer);
				}
				return true;
			}
			if(Sorter is GenRecordSorter) // If we already have a GenRecordSorter, it's probably an existing, valid one.
				return false;
			// Try to create a sorter based on the current Reversal Index's WritingSystem
			var newGuid = ReversalIndexEntryUi.GetObjectGuidIfValid(PropertyTable, "ReversalIndexGuid");
			if(newGuid.Equals(Guid.Empty))
				return false;
			var ri = Cache.ServiceLocator.GetObject(newGuid) as IReversalIndex;
			if(ri == null)
				return false;
			var writingSystem = (IWritingSystem)Cache.WritingSystemFactory.get_Engine(ri.WritingSystem);
			m_list.Sorter = new GenRecordSorter(new StringFinderCompare(LayoutFinder.CreateFinder(Cache, BrowseViewFormCol, fakevc,
																								PropertyTable.GetValue<IApp>("App")),
																		new WritingSystemComparer(writingSystem)));
			return true;
		}

		/// <summary>
		/// Receives the broadcast message "PropertyChanged"
		/// </summary>
		public override void OnPropertyChanged(string name)
		{
			CheckDisposed();

			var window = PropertyTable.GetValue<IFwMainWnd>("window");
			if (window != null)
			{
#if RANDYTODO
				// TODO: add to interface?
				window.ClearInvalidatedStoredData();
#endif
			}
			switch(name)
			{
				default:
					base.OnPropertyChanged(name);
					break;
				case "ReversalIndexGuid":
					ChangeOwningObjectIfPossible();
					break;
				case "ToolForAreaNamed_lexicon" :
					int rootIndex = GetRootIndex(m_list.CurrentIndex);
					JumpToIndex(rootIndex);
					base.OnPropertyChanged(name);
					break;
				case "ActiveClerk":
					RecordClerk activeClerk = PropertyTable.GetValue<RecordClerk>("ActiveClerk");
					if (activeClerk == this)
						ChangeOwningObjectIfPossible();
					else
						base.OnPropertyChanged(name);
					break;
			}
		}

		/// <summary>
		/// Returns the index of the root object whose descendent is the object at lastValidIndex.
		/// </summary>
		/// <param name="lastValidIndex"></param>
		/// <returns></returns>
		private int GetRootIndex(int lastValidIndex)
		{
			var item = m_list.SortItemAt(lastValidIndex);
			if (item == null)
				return lastValidIndex;
			var parentIndex = m_list.IndexOfParentOf(item.KeyObject);

			return parentIndex == -1 ? lastValidIndex : GetRootIndex(parentIndex);
		}

#if RANDYTODO
		/// <summary>
		/// This is enabled whenever the ReversalClerk is active.
		/// </summary>
		/// <param name="commandObject"></param>
		/// <param name="display"></param>
		/// <returns></returns>
		public virtual bool OnDisplayInsertReversalIndex(object commandObject, ref UIItemDisplayProperties display)
		{
			CheckDisposed();

			if (Cache == null)
			{
				display.Enabled = false;
			}
			else
			{
				int cRevIdx = Cache.LanguageProject.LexDbOA.ReversalIndexesOC.Count;
				int cWs = Cache.ServiceLocator.WritingSystems.AllWritingSystems.Count();
				display.Enabled = cRevIdx < cWs;
			}
			display.Visible = true;
			return true;
		}
#endif

		/// <summary />
		public virtual void OnInsertReversalIndex(object argument)
		{
			CheckDisposed();

			Guid newGuid = CreateNewReversalIndex();
			if (newGuid != Guid.Empty)
			{
				ChangeOwningObject(newGuid);
				var guid = ReversalIndexEntryUi.GetObjectGuidIfValid(PropertyTable, "ReversalIndexGuid");
				if (guid.Equals(Guid.Empty) || !guid.Equals(newGuid))
					SetReversalIndexGuid(newGuid);
			}
		}

		private Guid CreateNewReversalIndex()
		{
			if (Cache == null)
				return Guid.Empty;
			if (Cache.LanguageProject == null)
				return Guid.Empty;
			if (Cache.LanguageProject.LexDbOA == null)
				return Guid.Empty;
			using (CreateReversalIndexDlg dlg = new CreateReversalIndexDlg())
			{
				dlg.Init(Cache);
				// Don't bother if all languages already have a reversal index!
				if (dlg.PossibilityCount > 0)
				{
					if (dlg.ShowDialog(Form.ActiveForm) == DialogResult.OK)
					{
						int hvo = dlg.NewReversalIndexHvo;
						return Cache.ServiceLocator.GetObject(hvo).Guid;
					}
				}
			}
			return Guid.Empty;
		}

#if RANDYTODO
		/// <summary>
		/// This is enabled whenever the ReversalClerk is active.
		/// </summary>
		/// <param name="commandObject"></param>
		/// <param name="display"></param>
		/// <returns></returns>
		public virtual bool OnDisplayDeleteReversalIndex(object commandObject, ref UIItemDisplayProperties display)
		{
			CheckDisposed();

			if (Cache == null)
			{
				display.Enabled = false;
			}
			else
			{
				int cRevIdx = Cache.LanguageProject.LexDbOA.ReversalIndexesOC.Count;
				display.Enabled = cRevIdx > 0;
			}
			display.Visible = true;
			return true;
		}
#endif

		/// <summary />
		public virtual void OnDeleteReversalIndex(object argument)
		{
			CheckDisposed();

			var oldGuid = ReversalIndexEntryUi.GetObjectGuidIfValid(PropertyTable, "ReversalIndexGuid");
			if (oldGuid.Equals(Guid.Empty))
				return;

			if (Cache == null)
				return;
			IReversalIndex ri = (IReversalIndex)Cache.ServiceLocator.GetObject(oldGuid);
			DeleteReversalIndex(ri);
		}

		/// <summary />
		public void DeleteReversalIndex(IReversalIndex ri)
		{
			CheckDisposed();

			var mainWindow = PropertyTable.GetValue<Form>("window");
			using (new WaitCursor(mainWindow))
			{
				using (var dlg = new ConfirmDeleteObjectDlg(PropertyTable.GetValue<IHelpTopicProvider>("HelpTopicProvider")))
				{
					var ui = new CmObjectUi(ri);
					ui.InitializeFlexComponent(PropertyTable, Publisher, Subscriber);
					dlg.SetDlgInfo(ui, Cache, PropertyTable);
					dlg.TopMessage = LanguageExplorerResources.ksDeletingThisRevIndex;
					dlg.BottomQuestion = LanguageExplorerResources.ksReallyWantToDeleteRevIndex;
					if (DialogResult.Yes == dlg.ShowDialog(mainWindow))
						ReallyDeleteReversalIndex(ri);
				}
			}
		}

		/// <summary />
		protected virtual void ReallyDeleteReversalIndex(IReversalIndex ri)
		{
			try
			{
				Debug.Assert(ri.Hvo == m_list.OwningObject.Hvo);
				m_list.ListModificationInProgress = true;	// can't reload deleted list! (LT-5353)
				// We're about to do a MasterRefresh which clobbers the Undo stack,
				// so we might as well make this UOW not undoable
				NonUndoableUnitOfWorkHelper.Do(Cache.ActionHandlerAccessor,
					() =>
					{
						Cache.DomainDataByFlid.DeleteObj(ri.Hvo);
						int cobjNew;
						var idxNew = ReversalIndexAfterDeletion(Cache, out cobjNew);
						SetReversalIndexGuid(idxNew.Guid);
					});
				ChangeOwningObjectIfPossible();
			}
			finally
			{
				m_list.ListModificationInProgress = false;
			}
			// Without this, stale data can still display in the BulkEditSenses tool if you
			// recreate the deleted reversal index.
			Publisher.Publish("MasterRefresh", null);
		}

		/// <summary />
		internal static IReversalIndex ReversalIndexAfterDeletion(FdoCache cache, out int cobjNew)
		{
			IReversalIndex newIdx;
			cobjNew = cache.LanguageProject.LexDbOA.ReversalIndexesOC.Count;
			if (cobjNew == 0)
			{
				// Big trouble ensues if we don't have any reversal indexes at all, so ...
				// Create a reversal index for the current default analysis writing system.
				var riRepo = cache.ServiceLocator.GetInstance<IReversalIndexRepository>();
				newIdx = riRepo.FindOrCreateIndexForWs(cache.DefaultAnalWs);

				cobjNew = 1;
			}
			else
			{
				// Regardless, we need to change the reversal index hvo since the old one just
				// disappeared.
				newIdx = cache.LanguageProject.LexDbOA.ReversalIndexesOC.ToArray()[0];
			}
			return newIdx;
		}

		private void SetReversalIndexGuid(Guid reversalIndexGuid)
		{
			if (Cache.ServiceLocator.GetObject(reversalIndexGuid) is IReversalIndex)
			{
				PropertyTable.SetProperty("ReversalIndexGuid", reversalIndexGuid.ToString(), true, true);
			}
		}

		/// <summary />
		abstract protected ICmObject NewOwningObject(IReversalIndex ri);
	}

#if RANDYTODO
	// TODO: Only used in:
/*
<clerk id="AllReversalEntries">
	<dynamicloaderinfo assemblyPath="LanguageExplorer.dll" class="LanguageExplorer.Dumpster.ReversalEntryClerk" />
	<recordList owner="ReversalIndex" property="AllEntries">
		<dynamicloaderinfo assemblyPath="LanguageExplorer.dll" class="LanguageExplorer.Areas.Lexicon.AllReversalEntriesRecordList" />
	</recordList>
	<filters />
	<sortMethods>
		<sortMethod label="Form" assemblyPath="Filters.dll" class="SIL.FieldWorks.Filters.PropertyRecordSorter" sortProperty="ShortName" />
	</sortMethods>
</clerk>
*/
#endif
	/// <summary>
	/// This clerk is used to deal with the entries of a IReversalIndex.
	/// </summary>
	internal class ReversalEntryClerk : ReversalClerk
	{
		///// <summary>
		///// Contructor.
		///// </summary>
		///// <param name="id">Clerk id/name.</param>
		///// <param name="recordList">Record list for the clerk.</param>
		///// <param name="defaultSorter">The default record sorter.</param>
		///// <param name="defaultSortLabel"></param>
		///// <param name="defaultFilter">The default filter to use.</param>
		///// <param name="allowDeletions"></param>
		///// <param name="shouldHandleDeletion"></param>
		internal ReversalEntryClerk(IFdoServiceLocator serviceLocator, ISilDataAccessManaged decorator, IReversalIndex reversalIndex)
			: base("AllReversalEntries", new AllReversalEntriesRecordList(serviceLocator, decorator, reversalIndex), new PropertyRecordSorter("ShortName"), "Default", null, true, true)
		{
		}

		/// <summary />
		protected override ICmObject NewOwningObject(IReversalIndex ri)
		{
			return ri;
		}
	}
}