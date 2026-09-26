%{Cpp:LicenseTemplate}\
@if '%{Package}'
package %{Package};

@endif
@if '%{UseJni}' === 'true'
import java.io.File;
import java.security.CodeSource;

@endif
public class %{ClassName} {
@if '%{UseJni}' === 'true'
    static {
        try {
            CodeSource source = %{ClassName}.class.getProtectionDomain().getCodeSource();
            File jar = new File(source.getLocation().toURI());
            File library = new File(jar.getParentFile(), System.mapLibraryName("%{NativeTarget}"));
            System.load(library.getPath());
        } catch (Exception exception) {
            throw new ExceptionInInitializerError(exception);
        }
    }

    private static native String greetingFromCpp();

@endif
    public static void main(String[] args) {
@if '%{UseJni}' === 'true'
        String message = "Hello World from Java!";
        System.out.println(message);
        System.out.println(greetingFromCpp());
@else
        String message = "Hello World!";
        System.out.println(message);
@endif
    }
}
