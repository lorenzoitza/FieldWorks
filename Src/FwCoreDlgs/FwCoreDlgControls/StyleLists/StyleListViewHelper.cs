// Copyright (c) 2006-2013 SIL International
// This software is licensed under the LGPL, version 2.1 or later
// (http://www.gnu.org/licenses/lgpl-2.1.html)
//
// File: StyleListHelper.cs
// Responsibility: TE Team

using System;
using System.Windows.Forms;
using System.Collections;
using System.Diagnostics;
using System.Drawing;

using SIL.FieldWorks.Common.Controls;

namespace SIL.FieldWorks.FwCoreDlgControls
{
	/// ----------------------------------------------------------------------------------------
	/// <summary>
	///
	/// </summary>
	/// ----------------------------------------------------------------------------------------
	public class StyleListViewHelper : BaseStyleListHelper
	{
		private int m_styleColumn;

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Construct a new StyleListViewHelper for the given FwListView.
		/// </summary>
		/// <param name="listView">The given FwListView</param>
		/// <param name="styleColumn">The column that the style is shown in</param>
		/// ------------------------------------------------------------------------------------
		public StyleListViewHelper(FwListView listView, int styleColumn) : base(listView)
		{
			m_styleColumn = styleColumn;
			listView.DrawSubItem += new DrawListViewSubItemEventHandler(CtrlDrawSubItem);
			listView.SelectedIndexChanged += new EventHandler(CtrlSelectedIndexChanged);
		}

		#region Properties
		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Gets the control cast as an FwListView
		/// </summary>
		/// ------------------------------------------------------------------------------------
		private FwListView ListViewControl
		{
			get {return (FwListView)m_ctrl;}
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Gets the SelectedItem property from the style list box.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		public override StyleListItem SelectedStyle
		{
			get
			{
				if (ListViewControl.SelectedItems.Count > 0)
				{
					StyleListItem item = null;
					if (m_styleItemList.TryGetValue(ListViewControl.SelectedItems[0].Text, out item))
						return item;
				}
				return null;
			}
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Gets the Items property from the style list view.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		protected override ICollection Items
		{
			get {return ListViewControl.Items;}
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Gets or sets the list view's selected style by name.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		public override string SelectedStyleName
		{
			get
			{
				return (SelectedStyle != null ? SelectedStyle.ToString() : string.Empty);
			}
			set
			{
				if (value != null)
				{
					int index = -1;

					if (value != string.Empty)
						index = FindString(value);

					m_ignoreChosenDelegate = true;
					if (index != -1)
						ListViewControl.SelectedItems[index].Selected = true;
					else
						UnSelectAllItems();
					m_ignoreChosenDelegate = false;
				}
			}
		}
		#endregion

		#region List Control Delegate Methods
		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Draw the sub item.
		/// </summary>
		/// <param name="sender">The sender.</param>
		/// <param name="e">The <see cref="T:System.Windows.Forms.DrawListViewSubItemEventArgs"/>
		/// instance containing the event data.</param>
		/// ------------------------------------------------------------------------------------
		void CtrlDrawSubItem(object sender, DrawListViewSubItemEventArgs e)
		{
			if (e.ColumnIndex != m_styleColumn)
				return;

			Debug.Assert(e.ItemIndex != -1);

			ListViewItem item = e.Item;
			ListViewItem.ListViewSubItem subItem = e.SubItem;

			// Draw the icon only if we're not drawing a combo box's edit portion.
			int xOffset = 0;
			//if (e.ColumnIndex == m_styleColumn)

			//{
			// Get the item being drawn.
			StyleListItem styleItem;
			if (m_styleItemList.TryGetValue(subItem.Text, out styleItem))
			{
				// Determine what image to draw, considering the selection state of the item and
				// whether the item is a character style or a paragraph style.
				Image icon = GetCorrectIcon(styleItem, item.Selected && m_ctrl.Focused);

				e.Graphics.DrawImage(icon, e.Bounds.Left, e.Bounds.Top +
					(e.Bounds.Height - icon.Height) / 2);
				xOffset = icon.Width;
			}
			//}
			Rectangle bounds = e.Bounds;
			bounds.X += xOffset;
			bounds.Width -= xOffset;

			// Draw the item's text, considering the item's selection state. Item text in the
			// edit portion will be drawn further left than those in the drop-down because text
			// in the edit portion doesn't have the icon to the left.
			TextRenderer.DrawText(e.Graphics, subItem.Text, m_ctrl.Font, bounds, ListViewControl.GetTextColor(e),
				TextFormatFlags.LeftAndRightPadding | TextFormatFlags.SingleLine | TextFormatFlags.VerticalCenter);

			//e.Graphics.DrawString(subItem.Text, m_ctrl.Font,
			//    new SolidBrush(item.Selected ? SystemColors.HighlightText : SystemColors.WindowText),
			//    bounds);
		}

		#endregion

		#region Public Methods
		/// ------------------------------------------------------------------------------------
		/// <summary>
		///
		/// </summary>
		/// <param name="str"></param>
		/// <returns></returns>
		/// ------------------------------------------------------------------------------------
		public int FindString(string str)
		{
			for (int i = 0; i < ListViewControl.Items.Count; i++)
			{
				if (((StyleListItem)ListViewControl.Items[i].Tag).ToString() == str)
					return i;
			}
			return -1;
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// UnSelects all items in the list view
		/// </summary>
		/// ------------------------------------------------------------------------------------
		public void UnSelectAllItems()
		{
			for (int i = 0; i < ListViewControl.Items.Count; i++)
				ListViewControl.Items[i].Selected = false;
		}

		/// <summary>
		/// Add item during refresh.
		/// </summary>
		/// <param name="items"></param>
		protected override void UpdateStyleList(StyleListItem[] items)
		{
			// do nothing for this method
		}
		#endregion
	}
}
