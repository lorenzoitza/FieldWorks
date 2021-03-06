// Copyright (c) 2003-2013 SIL International
// This software is licensed under the LGPL, version 2.1 or later
// (http://www.gnu.org/licenses/lgpl-2.1.html)
//
// File: AppHandle.cs
// Responsibility: LastufkaM
// Last reviewed:
//
// <remarks>
// </remarks>

using System;
using System.Diagnostics;
using System.Threading;
using System.Xml;
using System.Windows.Forms;
using NUnit.Framework;
using System.Runtime.InteropServices; // DllImport

namespace GuiTestDriver
{
	/// <summary>
	/// AppHandle accesses the application in various ways.
	/// It needs to be independent of Application Context so it
	/// can be passed to the executing instruction.
	/// </summary>
	public class AppHandle
	{
		[DllImport("user32.dll", CharSet = CharSet.Auto)]
		static public extern bool SetForegroundWindow(IntPtr hWnd);

		string m_ExePath;
		Process m_proc;
		AccessibilityHelper m_AccHelper;

		public AppHandle()
		{
			m_ExePath      = null;
			m_proc         = null;
			m_AccHelper    = null;
		}
		public AppHandle(string exe)
		{
			m_ExePath      = exe;
			m_proc         = null;
			m_AccHelper    = null;
		}
		public AppHandle(string exe, Process proc, AccessibilityHelper ah)
		{
			m_ExePath      = exe;
			m_proc         = proc;
			m_AccHelper    = ah;
		}

		public string ExePath
		{
			get {return m_ExePath;}
			set {Assert.Fail("ExePath can only be set on subclasses");}
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Get the process associated with the application
		/// </summary>
		/// ------------------------------------------------------------------------------------
		public Process Process
		{
			get { m_proc.Refresh(); return m_proc; }
			set { m_proc = value; }
		}

		public AccessibilityHelper MainAccessibilityHelper
		{
			get
			{
				if (m_AccHelper == null && m_proc != null)
				{
					m_proc.Refresh();
					m_AccHelper = new AccessibilityHelper(m_proc.MainWindowHandle);
				}
				return m_AccHelper;
			}
			set { m_AccHelper = value; }
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Send key strokes to the application.
		/// </summary>
		/// <param name="keys">Key strokes to send</param>
		/// <seealso cref="SendKeys"/>
		/// ------------------------------------------------------------------------------------
		public void SendKeys(string keys)
		{
			m_proc.Refresh();
			if (m_proc.HasExited)
				return;

			m_proc.WaitForInputIdle(30000);
			System.Windows.Forms.SendKeys.SendWait(keys);

			m_proc.Refresh();
			if (m_proc.HasExited)
				return;
			m_proc.WaitForInputIdle(30000);
		}
		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Exits the application
		/// </summary>
		/// <param name="fOnCurrentWindow">If <c>true</c> send Alt-F4 regardless of what window
		/// is active. Otherwise switch to the main window first.</param>
		/// ------------------------------------------------------------------------------------
		public void Exit(bool fOnCurrentWindow)
		{ // assume m_proc != null - an assert would have come up during launch in on-application
			m_proc.Refresh();
			m_proc.WaitForInputIdle();
			if (!fOnCurrentWindow)
			{
				try { m_proc.Kill(); }
				catch {}
			}
			else KillWithKeys(fOnCurrentWindow);
			m_proc.Refresh();
			m_proc.WaitForExit(3000);
			m_proc = null;
			m_AccHelper = null;
		}
		private void KillWithKeys(bool fOnCurrentWindow)
		{
			try
			{
				if (!fOnCurrentWindow)
				{
					m_proc.Refresh();
					SetForegroundWindow(m_proc.MainWindowHandle);
				}
				// It works better if we send an ESC first
				SendKeys("{ESC}");
				SendKeys("%{F4}");
			}
			catch {}
		}

	}
}
