// Copyright (c) 2010-2013 SIL International
// This software is licensed under the LGPL, version 2.1 or later
// (http://www.gnu.org/licenses/lgpl-2.1.html)
//
// File:
// Responsibility:
//
// <remarks>
// </remarks>
// ---------------------------------------------------------------------------------------------

using System.Windows.Forms;
using NUnit.Framework;
using SIL.FieldWorks.FDO.FDOTests;

namespace SIL.FieldWorks.FwCoreDlgControls
{
	/// ----------------------------------------------------------------------------------------
	/// <summary>
	/// Tests for DefaultFontsControl, a control used in the Writing System Properties dialog
	/// </summary>
	/// ----------------------------------------------------------------------------------------
	[TestFixture]
	public class DefaultFontsControlTests : MemoryOnlyBackendProviderRestoredForEachTestTestBase
	{
		/// <summary>object for testing.</summary>
		DefaultFontsControl m_fontsControl;

		/// ----------------------------------------------------------------------------------------
		/// <summary>
		/// Initialize for tests.
		/// </summary>
		/// ----------------------------------------------------------------------------------------
		public override void TestSetup()
		{
			base.TestSetup();

			m_fontsControl = new DefaultFontsControl();
		}

		/// ----------------------------------------------------------------------------------------
		/// <summary>
		/// Tear down after tests.
		/// </summary>
		/// ----------------------------------------------------------------------------------------
		public override void TestTearDown()
		{
			base.TestTearDown();

			m_fontsControl.Dispose();
			m_fontsControl = null;
		}

		/// ----------------------------------------------------------------------------------------
		/// <summary>
		/// Make sure font names are alphabetically sorted in control.
		/// Related to FWNX-273: Fonts not in alphabetical order
		/// </summary>
		/// ----------------------------------------------------------------------------------------
		[Test]
		public void FontsAreAlphabeticallySorted()
		{
			var fontNames_Normal = ((ComboBox) GetField(m_fontsControl, "m_defaultFontComboBox")).Items;

			for (var i = 0; i+1 < fontNames_Normal.Count; i++)
			{
				// Check that each font in the list is alphabetically before the next font in the list
				Assert.LessOrEqual(fontNames_Normal[i] as string, fontNames_Normal[i+1] as string, "Font names not alphabetically sorted.");
			}
		}
	}
}