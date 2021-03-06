// Copyright (c) 2010-2013 SIL International
// This software is licensed under the LGPL, version 2.1 or later
// (http://www.gnu.org/licenses/lgpl-2.1.html)
//
// File: FixedOrphanFootnoteReportDlg.cs
// Responsibility: TE Team

using System.Collections.Generic;
using SIL.FieldWorks.Common.FwUtils;
using SIL.FieldWorks.FwCoreDlgs;

namespace SIL.FieldWorks.TE
{
	/// ----------------------------------------------------------------------------------------
	/// <summary>
	/// Message box displayed when TE has detected problems with orphaned ORC and/or footnotes.
	/// </summary>
	/// ----------------------------------------------------------------------------------------
	public partial class FixedOrphanFootnoteReportDlg : FwUpdateReportDlg
	{
		#region Constructors
		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Initializes a new instance of the <see cref="T:FixedOrphanFootnoteReportDlg"/> class.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		public FixedOrphanFootnoteReportDlg()
		{
			InitializeComponent();
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Initializes a new instance of the <see cref="T:FixedOrphanFootnoteReportDlg"/> class.
		/// </summary>
		/// <param name="issues">List of styles that the user has modified.</param>
		/// <param name="projectName">Name of the project.</param>
		/// <param name="helpTopicProvider">context sensitive help</param>
		/// ------------------------------------------------------------------------------------
		public FixedOrphanFootnoteReportDlg(List<string> issues, string projectName,
			IHelpTopicProvider helpTopicProvider) :
			base(issues, projectName, helpTopicProvider)
		{
			InitializeComponent();
			RepeatTitleOnEveryPage = true;
			RepeatColumnHeaderOnEveryPage = false;
		}
		#endregion

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Gets the help topic key.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		protected override string HelpTopicKey
		{
			get { return "khtpProblemsWithFootnotesOrPicturesDialog"; }
		}
	}
}