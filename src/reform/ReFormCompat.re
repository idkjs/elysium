module Value = {
  type t = string;
};

module Helpers = {
  let handleDomFormChange = (handleChange, event) =>
    handleChange(ReactEvent.Form.target(event)##value);
  let handleDomFormSubmit = (handleSubmit, event) => {
    ReactEvent.Synthetic.preventDefault(event);
    handleSubmit();
  };
};

module Validation = {
  /* Here the possible accepted validadors are defined */
  /* Also their respective validation logic */
  module I18n = {
    type dictionary = {
      required: string,
      email: string,
    };
    let ptBR = {
      required: {j|Campo obrigatório|j},
      email: {j|Email inválido|j},
    };
    let en = {required: "Field is required", email: "Invalid email"};
  };

  type validation('values) =
    | Required
    | Email
    | Custom('values => option(string));

  let getValidationError =
      ((_, validator), ~values, ~value, ~i18n: I18n.dictionary) =>
    switch (validator) {
    | Required => String.length(value) < 1 ? Some(i18n.required) : None
    | Custom(fn) => fn(values)
    | Email =>
      Js.Re.test(value, [%bs.re {|/\S+@\S+\.\S+/|}])
        ? None : Some(i18n.email)
    };
};

module ReForm = {
  /* Validation types */
  let safeHd = lst => List.length(lst) == 0 ? None : Some(List.hd(lst));

  let (>>=) = (value, map) =>
    switch (value) {
    | None => None
    | Some(value) => map(value)
    };

  module type Config = {
    type state;
    type fields;
    /* (fieldName, getter, setter) */
    let lens: list((fields, state => Value.t, (state, Value.t) => state));
  };

  module Create = (Config: Config) => {
    /* TODO: Make a variant out of this */
    type value = Value.t;
    /* Form actions */
    type action =
      | TrySubmit
      | HandleSubmitting(bool)
      | SetFieldsErrors(list((Config.fields, option(string))))
      | HandleFieldValidation((Config.fields, value))
      | HandleError(option(string))
      | HandleChange((Config.fields, value))
      | HandleSubmit
      | ResetFormState
      | HandleSetFocusedField(Config.fields)
      | HandleUnsetFocusedField;
    type values = Config.state;
    type schema = list((Config.fields, Validation.validation(values)));
    module Field = {
      let getFieldLens:
        Config.fields =>
        (
          Config.fields,
          Config.state => value,
          (Config.state, value) => Config.state,
        ) =
        field =>
          /* TODO handle exception */
          Config.lens
          |> List.find(((fieldName, _, _)) => fieldName === field);
      let validateField:
        (Config.fields, values, value, schema, Validation.I18n.dictionary) =>
        option(string) =
        (field, values, value, schema, i18n) =>
          schema
          |> List.filter(((fieldName, _)) => fieldName === field)
          |> safeHd
          >>= (
            fieldSchema =>
              Validation.getValidationError(
                fieldSchema,
                ~values,
                ~value,
                ~i18n,
              )
          );
      let handleChange: ((Config.fields, value), values) => values =
        ((field, value), values) => {
          let (_, _, setter) = getFieldLens(field);
          setter(values, value);
        };
    };
    type onSubmit = {
      values,
      setSubmitting: bool => unit,
      setError: option(string) => unit,
      resetFormState: unit => unit,
    };
    type state = {
      values,
      isSubmitting: bool,
      errors: list((Config.fields, option(string))),
      error: option(string),
      focusedField: option(Config.fields),
    };
    /* Type of what is given to the children */
    type reform = {
      form: state,
      handleChange: (Config.fields, value) => unit,
      handleGlobalValidation: option(string) => unit,
      handleSubmit: unit => unit,
      getErrorForField: Config.fields => option(string),
      resetFormState: unit => unit,
      setFocusedField: Config.fields => unit,
      unsetFocusedField: unit => unit,
      focusedField: option(Config.fields),
    };
    let component = ReasonReact.reducerComponent("ReForm");

    [@react.component]
    let make =
        (
          ~onSubmit: onSubmit => unit,
          ~onSubmitFail: list((Config.fields, option(string))) => unit=ignore,
          ~onFormStateChange: state => unit=ignore,
          ~validate: values => option(string)=_ => None,
          ~initialState: Config.state,
          ~schema: schema,
          ~i18n: Validation.I18n.dictionary=Validation.I18n.en,
          ~children,
        ) =>
      ReactCompat.useRecordApi({
        ...component,
        initialState: () => {
          values: initialState,
          error: None,
          isSubmitting: false,
          errors: [],
          focusedField: None,
        },
        reducer: (action, state) =>
          switch (action) {
          | TrySubmit =>
            SideEffects(
              self => {
                let globalValidationError = validate(self.state.values);
                let fieldsValidationErrors =
                  schema
                  |> List.map(((fieldName, _)) => {
                       let (_, getter, _) = Field.getFieldLens(fieldName);
                       (
                         fieldName,
                         Field.validateField(
                           fieldName,
                           self.state.values,
                           getter(self.state.values),
                           schema,
                           i18n,
                         ),
                       );
                     })
                  |> List.filter(((_, fieldError)) => fieldError !== None);
                self.send(SetFieldsErrors(fieldsValidationErrors));
                self.send(HandleError(globalValidationError));
                globalValidationError === None
                && List.length(fieldsValidationErrors) == 0
                  ? self.send(HandleSubmit)
                  : onSubmitFail(fieldsValidationErrors);
              },
            )
          | ResetFormState =>
            UpdateWithSideEffects(
              {
                ...state,
                values: initialState,
                errors: [],
                isSubmitting: false,
              },
              self => onFormStateChange(self.state),
            )
          | HandleSubmitting(isSubmitting) =>
            UpdateWithSideEffects(
              {...state, isSubmitting},
              self => onFormStateChange(self.state),
            )
          | HandleError(error) =>
            UpdateWithSideEffects(
              {...state, isSubmitting: false, error},
              self => onFormStateChange(self.state),
            )
          | SetFieldsErrors(errors) =>
            UpdateWithSideEffects(
              {...state, isSubmitting: false, errors},
              self => onFormStateChange(self.state),
            )
          | HandleFieldValidation((field, value)) =>
            UpdateWithSideEffects(
              {
                ...state,
                errors:
                  state.errors
                  |> List.filter(((fieldName, _)) => fieldName !== field)
                  |> List.append([
                       (
                         field,
                         Field.validateField(
                           field,
                           state.values,
                           value,
                           schema,
                           i18n,
                         ),
                       ),
                     ]),
              },
              self => onFormStateChange(self.state),
            )
          | HandleChange((field, value)) =>
            UpdateWithSideEffects(
              {
                ...state,
                values: Field.handleChange((field, value), state.values),
              },
              self => self.send(HandleFieldValidation((field, value))),
            )
          | HandleSubmit =>
            UpdateWithSideEffects(
              {...state, isSubmitting: true},
              self => {
                onSubmit({
                  resetFormState: () => self.send(ResetFormState),
                  values: state.values,
                  setSubmitting: isSubmitting =>
                    self.send(HandleSubmitting(isSubmitting)),
                  setError: error => self.send(HandleError(error)),
                });
                onFormStateChange(self.state);
              },
            )
          | HandleSetFocusedField(focusedField) =>
            UpdateWithSideEffects(
              {...state, focusedField: Some(focusedField)},
              self => onFormStateChange(self.state),
            )
          | HandleUnsetFocusedField =>
            UpdateWithSideEffects(
              {...state, focusedField: None},
              self => onFormStateChange(self.state),
            )
          },
        render: self => {
          let handleChange = (field, value) =>
            self.send(HandleChange((field, value)));
          let handleGlobalValidation = error =>
            self.send(HandleError(error));
          let handleSubmit = _ => self.send(TrySubmit);
          let resetFormState = _ => self.send(ResetFormState);
          let getErrorForField: Config.fields => option(string) =
            field =>
              self.state.errors
              |> List.filter(((fieldName, _)) => fieldName === field)
              |> List.map(((_, error)) => error)
              |> safeHd
              >>= (i => i);
          let setFocusedField = field =>
            self.send(HandleSetFocusedField(field));
          let unsetFocusedField = () => self.send(HandleUnsetFocusedField);
          children({
            form: self.state,
            handleChange,
            handleSubmit,
            handleGlobalValidation,
            getErrorForField,
            resetFormState,
            setFocusedField,
            unsetFocusedField,
            focusedField: self.state.focusedField,
          });
        },
      });
  };
};