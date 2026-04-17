
float redundantInstructions(int varExt) {
    int a = varExt*4;
    float q = a/30;
    int b = a - 2;
    int aux = a - 3;
    
    int c = b + 2; // deve diventare a
    int d = 3 + aux; // deve diventare a (2+ (a-2) )
    
    int pluto = c+4; // di controllo
    int z = varExt*45;
    int paperino = 7 - d; // di controllo

    float controZ = z/45; // deve diventare z
    int e = 2 + b; // deve diventare a (2 + (a-2))
    return a+q+b+aux+c+d+pluto+z+paperino+controZ+e;
}
/*
%2 = a
%4 = q
%5 = b
%6 = aux
%7 = c = b + 2 -> %5, 2 -> %2
%8 = d = 3 - aux -> 3, %6 -> %2
*/

float redundantDivOrMult(int varExt) {
    int z = varExt*45;
    int z2 = 35*varExt;

    // controllo interruzioni
    int a = varExt*4;
    float q = z/30;

    float controZ = z/45; // deve diventare varExt (%0)
    float controZ2 = z2*35; // deve diventare varExt (%0)


    // controllo interruzioni
    q = 1 + controZ;
    float j = 1 + z;
    float h = 1 + controZ2;


    return z+z2+a+q+controZ+controZ2+q+j+h;

}