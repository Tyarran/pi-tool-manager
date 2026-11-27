type t =
  | NotConfigured
  | Invalid(string)
  | Active(Config.t, Session.t);

let current = ref(NotConfigured);

let get = () => current^;

let set = state => current := state;

let fromSettings = settings =>
  switch (Config.decode(Js.Dict.get(settings, Config.key))) {
  | Ok(Some(config)) => Active(config, Session.start(config))
  | Ok(None) => NotConfigured
  | Error(message) => Invalid(message)
  };

let selection = () =>
  switch (get()) {
  | Active(config, session) => Session.selection(config, session)
  | NotConfigured
  | Invalid(_) => Names.Set.empty
  };

let unavailableReason = state =>
  switch (state) {
  | Active(_) => None
  | NotConfigured => Some(Config.key ++ " is not configured")
  | Invalid(message) =>
    Some(
      Printf.sprintf(
        "%s is disabled, fix its configuration: %s",
        Config.key,
        message,
      ),
    )
  };
