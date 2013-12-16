// Copyright (c) 2009-2013 SIL International
// This software is licensed under the LGPL, version 2.1 or later
// (http://www.gnu.org/licenses/lgpl-2.1.html)
//
// File: ExemplarCharactersHelperTests.cs
// Responsibility: TE Team
// ---------------------------------------------------------------------------------------------
using NUnit.Framework;
using SIL.FieldWorks.Test.TestUtils;
using SIL.Utils;

namespace SIL.FieldWorks.Common.FwUtils
{
	/// ----------------------------------------------------------------------------------------
	/// <summary>
	/// Tests for ExemplarCharactersHelper.
	/// </summary>
	/// ----------------------------------------------------------------------------------------
	[TestFixture]
	public class ExemplarCharactersHelperTests : BaseTest
	{
		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Tests GetValidCharsForLocale for a simple set of characters.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[Test]
		public void GetValidCharsForLocale_Simple()
		{
			DummyICU icu = new DummyICU();
			DummyCharPropEngine cpe = new DummyCharPropEngine();
			icu.m_icu.SetupResultForParams("GetExemplarCharacters", "[a b c]", "qwe");
			ReflectionHelper.SetField(typeof(ExemplarCharactersHelper), "s_ICU", icu);
			Assert.AreEqual("  a b c A B C",
				ExemplarCharactersHelper.GetValidCharsForLocale("qwe", cpe));

			icu.m_icu.Verify();
			cpe.Verify();
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Tests GetValidCharsForLocale for a range of characters.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[Test]
		public void GetValidCharsForLocale_Range()
		{
			DummyICU icu = new DummyICU();
			DummyCharPropEngine cpe = new DummyCharPropEngine();
			icu.m_icu.SetupResultForParams("GetExemplarCharacters", "[a-c]", "qwe");
			ReflectionHelper.SetField(typeof(ExemplarCharactersHelper), "s_ICU", icu);
			Assert.AreEqual("  a b c A B C",
				ExemplarCharactersHelper.GetValidCharsForLocale("qwe", cpe));
			icu.m_icu.Verify();
			cpe.Verify();
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Tests GetValidCharsForLocale for a digraph.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[Test]
		public void GetValidCharsForLocale_Digraph()
		{
			DummyICU icu = new DummyICU();
			DummyCharPropEngine cpe = new DummyCharPropEngine();
			icu.m_icu.SetupResultForParams("GetExemplarCharacters", "[{ch}]", "qwe");
			ReflectionHelper.SetField(typeof(ExemplarCharactersHelper), "s_ICU", icu);
			Assert.AreEqual("  ", ExemplarCharactersHelper.GetValidCharsForLocale("qwe", cpe));
			icu.m_icu.Verify();
			cpe.Verify();
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Tests GetValidCharsForLocale for a range of characters.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[Test]
		public void GetValidCharsForLocale_Complex()
		{
			DummyICU icu = new DummyICU();
			DummyCharPropEngine cpe = new DummyCharPropEngine();
			icu.m_icu.SetupResultForParams("GetExemplarCharacters", "[a-c {ch} de f-g e\u0301 \u0301]", "qwe");
			ReflectionHelper.SetField(typeof(ExemplarCharactersHelper), "s_ICU", icu);
			Assert.AreEqual("  a b c f g e\u0301 A B C F G E\u0301",
				ExemplarCharactersHelper.GetValidCharsForLocale("qwe", cpe));
			icu.m_icu.Verify();
			cpe.Verify();
		}
	}
}
