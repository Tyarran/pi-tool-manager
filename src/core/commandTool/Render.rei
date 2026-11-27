let argv: (Spec.t, Arguments.t, ~home: string) => list(string);

let env: (Spec.t, Arguments.t, ~home: string) => list((string, string));

let workingDirectory: (Spec.t, ~home: string) => option(string);
