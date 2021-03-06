// Copyright (c) 2013 SIL International
// This software is licensed under the LGPL, version 2.1 or later
// (http://www.gnu.org/licenses/lgpl-2.1.html)


using Microsoft.Win32;

namespace SIL.FieldWorks.Common.FwUtils
{
	/// <summary>
	/// Helper class for accessing FieldWorks-specific registry settings. Extracted as interface
	/// so that unit tests can provide alternative implementation.
	/// </summary>
	public interface IFwRegistryHelper
	{
		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Gets the read-only local machine Registry key for FieldWorks.
		/// NOTE: This key is not opened for write access because it will fail on
		/// non-administrator logins.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		RegistryKey FieldWorksRegistryKeyLocalMachine { get; }

		/// <summary>
		/// Get LocalMachine hive. (Overridable for unit tests.)
		/// </summary>
		RegistryKey LocalMachineHive { get; }

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Gets the read-only local machine Registry key for FieldWorksBridge.
		/// NOTE: This key is not opened for write access because it will fail on
		/// non-administrator logins.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		RegistryKey FieldWorksBridgeRegistryKeyLocalMachine { get; }

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Gets the local machine Registry key for FieldWorks.
		/// NOTE: This will throw with non-administrative logons! Be ready for that.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		RegistryKey FieldWorksRegistryKeyLocalMachineForWriting { get; }

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Gets the default (current user) Registry key for FieldWorks.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		RegistryKey FieldWorksRegistryKey { get; }

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Gets the default (current user) Registry key for FieldWorks without the version number.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		RegistryKey FieldWorksVersionlessRegistryKey { get; }

		/// <summary>
		/// The value we look up in the FieldWorksRegistryKey to get(or set) the persisted user locale.
		/// </summary>
		string UserLocaleValueName { get; }

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Determines the installation or absence of the Paratext program by checking for the
		/// existence of the registry key that that application uses to store its program files
		/// directory in the local machine settings.
		/// This is 'HKLM\Software\ScrChecks\1.0\Program_Files_Directory_Ptw(7,8,9)'
		/// NOTE: This key is not opened for write access because it will fail on
		/// non-administrator logins.
		///
		/// </summary>
		/// ------------------------------------------------------------------------------------
		bool Paratext7orLaterInstalled();
	}
}
