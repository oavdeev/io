DOServer := Server clone do(
//metadoc DOServer category Networking
/*metadoc DOServer description
The DOServer object is usefull for fetching web pages and parsing DOServers. Example;
<pre>
Test := Object clone
Test test := method(v, 
	write("got test '", v, "'\n")
	return List clone append(1)
)

doServer := DOServer clone
doServer setRootObject(Test clone)
doServer setPort(8456)
doServer start
</pre>
*/

	//doc DOServer setRootObject(anObject) Sets the root object which incoming messages will be send to.
	rootObject ::= nil
	handleSocket := method(aSocket,
		con := DOConnection clone
		con localObjects atPut("0", rootObject)
		con @handleSocket(aSocket)
	)
)
