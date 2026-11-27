type theme;

type component;

type color = [
  | `toolTitle
  | `accent
  | `muted
  | `dim
];

let fg: (theme, color, string) => string;

let bold: (theme, string) => string;

let text: string => component;
