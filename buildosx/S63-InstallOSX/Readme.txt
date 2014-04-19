Description files from o-charts.org (*.rtf extension)

To be changed for the local environment:
/Users/heidi/devel/Cocoa-OCPN/plugins/S63_pi/Setups/Install-OSX/

Included javascript:

<dict>
		<key>IC_REQUIREMENT_JAVASCRIPT_SHARED_SOURCE_CODE</key>
		<string>function checkForOCPN() { 
var chk_flag = false; 
var ocpn_vers;try {// is OpenCPN installed?
 chk_flag = system.files.fileExistsAtPath('/Applications/OpenCPN.app');if (chk_flag) {// is it the right version of OpenCPN?
  ocpn_data = system.files.bundleAtPath('/Applications/OpenCPN.app');ocpn_vers = ocpn_data.CFBundleShortVersionString;chk_flag = (ocpn_vers == '3.3.1606'); }} catch (e) {// an exception just occurred
 return (false)}// return the check results
  return chk_flag;}
     </string>
	</dict>
	<key>TYPE</key>
	<integer>0</integer>
	<key>VERSION</key>
	<integer>2</integer>
</dict>
