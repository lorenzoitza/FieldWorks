using System.Xml.Linq;

namespace SIL.FieldWorks.LexText.Controls
{
	/// <summary>
	/// Interface for parser trace processing
	/// </summary>
	public interface IParserTrace
	{
		/// <summary>
		/// Create an HTML page of the results
		/// </summary>
		/// <param name="result">XML of the parse trace output</param>
		/// <param name="isTrace"></param>
		/// <returns>URL of the resulting HTML page</returns>
		string CreateResultPage(XDocument result, bool isTrace);
	}
}