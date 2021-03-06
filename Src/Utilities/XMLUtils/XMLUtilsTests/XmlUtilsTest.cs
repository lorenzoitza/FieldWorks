// Copyright (c) 2003-2013 SIL International
// This software is licensed under the LGPL, version 2.1 or later
// (http://www.gnu.org/licenses/lgpl-2.1.html)
//
// File: XmlUtilsTest.cs
// Responsibility: Andy Black
// Last reviewed:
//
// <remarks>
// </remarks>

using NUnit.Framework;

namespace SIL.Utils
{
	/// <summary>
	/// Summary description for XmlUtilsTest.
	/// </summary>
	[TestFixture]
	public class XmlUtilsTest : TestBaseForTestsThatCreateTempFilesBasedOnResources
	{
		/// <summary>
		/// Location of test files
		/// </summary>
		protected string m_sTestPath;

		[TestFixtureSetUp]
		public void FixtureSetUp()
		{
			m_sTestPath = CreateTempTestFiles(typeof(Properties.Resources), "TransformTestFiles");
		}

		/// <summary>
		///
		/// </summary>
		[Test]
		public void GetBooleanAttributeValueTest()
		{
			// All in this section should return false.
			Assert.IsFalse(XmlUtils.GetBooleanAttributeValue(null), "Null returns false");
			Assert.IsFalse(XmlUtils.GetBooleanAttributeValue("FALSE"), "'FALSE' returns false");
			Assert.IsFalse(XmlUtils.GetBooleanAttributeValue("False"), "'False' returns false");
			Assert.IsFalse(XmlUtils.GetBooleanAttributeValue("false"), "'false' returns false");
			Assert.IsFalse(XmlUtils.GetBooleanAttributeValue("NO"), "'NO' returns false");
			Assert.IsFalse(XmlUtils.GetBooleanAttributeValue("No"), "'No' returns false");
			Assert.IsFalse(XmlUtils.GetBooleanAttributeValue("no"), "'no' returns false");

			// All in this section should return true.
			Assert.IsTrue(XmlUtils.GetBooleanAttributeValue("TRUE"), "'TRUE' returns true");
			Assert.IsTrue(XmlUtils.GetBooleanAttributeValue("True"), "'True' returns true");
			Assert.IsTrue(XmlUtils.GetBooleanAttributeValue("true"), "'true' returns true");
			Assert.IsTrue(XmlUtils.GetBooleanAttributeValue("YES"), "'YES' returns true");
			Assert.IsTrue(XmlUtils.GetBooleanAttributeValue("Yes"), "'Yes' returns true");
			Assert.IsTrue(XmlUtils.GetBooleanAttributeValue("yes"), "'yes' returns true");
		}

		[Test]
		public void MakeSafeXmlAttributeTest()
		{
			string sFixed = XmlUtils.MakeSafeXmlAttribute("abc&def<ghi>jkl\"mno'pqr&stu");
			Assert.AreEqual("abc&amp;def&lt;ghi&gt;jkl&quot;mno&apos;pqr&amp;stu", sFixed, "First Test of MakeSafeXmlAttribute");

			sFixed = XmlUtils.MakeSafeXmlAttribute("abc&def\r\nghi\u001Fjkl\u007F\u009Fmno");
			Assert.AreEqual("abc&amp;def&#xD;&#xA;ghi&#x1F;jkl&#x7F;&#x9F;mno", sFixed, "Second Test of MakeSafeXmlAttribute");
		}

		[Test]
		public void DecodeXmlAttributeTest()
		{
			string sFixed = XmlUtils.DecodeXmlAttribute("abc&amp;def&lt;ghi&gt;jkl&quot;mno&apos;pqr&amp;stu");
			Assert.AreEqual("abc&def<ghi>jkl\"mno'pqr&stu", sFixed, "First Test of DecodeXmlAttribute");

			sFixed = XmlUtils.DecodeXmlAttribute("abc&amp;def&#xD;&#xA;ghi&#x1F;jkl&#x7F;&#x9F;mno");
			Assert.AreEqual("abc&def\r\nghi\u001Fjkl\u007F\u009Fmno", sFixed, "Second Test of DecodeXmlAttribute");
		}
	}
}
