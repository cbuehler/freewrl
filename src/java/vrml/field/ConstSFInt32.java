//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.BufferedReader;
import java.io.PrintWriter;
import java.io.IOException;

public class ConstSFInt32 extends ConstField {
     int value;

    public ConstSFInt32() { }

    public ConstSFInt32(int value) {
	        this.value = value;
    }

    public int getValue() {
        __updateRead();
        return value;
    }

    public String toString() {
        __updateRead();
        return String.valueOf(value);
    }

    public void __fromPerl(BufferedReader in)  throws IOException {

	//System.out.println ("fromPerl, Int32");
		value = Integer.parseInt(in.readLine());
    }

    public void __toPerl(PrintWriter out)  throws IOException {
        out.print(value);
	//out.println();
    }
    //public void setOffset(String offs) { this.offset = offs; } //JAS2
    //public String getOffset() { return this.offset; } //JAS2
}