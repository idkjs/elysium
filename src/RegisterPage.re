type state = {isSubmitting: bool};

type actions =
  | SubmitForm
  | FinishSubmit;

let component = ReasonReact.reducerComponent("ProjectForm");

open Types;

module Register = [%graphql
  {| mutation register($email: String!, $password: String!) {
       register(email: $email, password: $password) {
         token
         user {
           id
           displayName
         }
       }
  }
  |}
];

module RegisterMutation = ReasonApollo.CreateMutation(Register);

module RegisterParams = {
  type state = {
    email: string,
    password: string,
    passwordConfirm: string,
  };
  type fields = [ | `email | `password | `passwordConfirm];
  let lens = [
    (`email, s => s.email, (s, email) => {...s, email}),
    (`password, s => s.password, (s, password) => {...s, password}),
    (`passwordConfirm, s => s.passwordConfirm, (s, passwordConfirm) => {...s, passwordConfirm}),
  ];
};

open ReasonApolloTypes;
module RegisterForm = ReForm.Create(RegisterParams);
exception GraphQLErrors(array(graphqlError));
exception EmptyResponse;

let make = (~createSession, _children) => {
  ...component,
  initialState: () => {isSubmitting: false},
  reducer: (action, _state) =>
    switch (action) {
    | SubmitForm => ReasonReact.Update({isSubmitting: true})
    | FinishSubmit => ReasonReact.Update({isSubmitting: false})
    },
  render: self => {
    <div>
      <h1> {ReasonReact.string("Sign Up")} </h1>
      <RegisterMutation>
        ...{(mutation, _) =>
          <RegisterForm
            onSubmit={({values}) => {
              Js.log("SUBMITTNG");
              self.send(SubmitForm);
              let registerQuery = Register.make(~email=values.email, ~password=values.password, ());
              mutation(~variables=registerQuery##variables, ())
              |> Js.Promise.then_(res => {
                   self.ReasonReact.send(FinishSubmit);
                   switch (res) {
                   | Data(data) =>
                     let user = userFromJs(data##register##user);
                     let session = {token: data##register##token, user};
                     createSession(session);
                     Js.Promise.resolve();
                   | Errors(errs) =>
                     Js.log(errs);
                     Js.Promise.reject(raise(GraphQLErrors(errs)));
                   | EmptyResponse => Js.Promise.reject(raise(EmptyResponse))
                   };
                 })
              |> Js.Promise.catch(err => Js.Promise.resolve(Js.log(err)))
              |> ignore;
              ();
            }}
            initialState={email: "", password: "", passwordConfirm: ""}
            schema=[
              (`email, Email),
              (`password, Required),
              (
                `passwordConfirm,
                Custom(
                  values =>
                    values.password != values.passwordConfirm ?
                      Some("Password confirmation must be same as password") : None,
                ),
              ),
            ]>
            ...{({handleSubmit, handleChange, form, getErrorForField}) =>
              <form onSubmit={ReForm.Helpers.handleDomFormSubmit(handleSubmit)}>
                <Input
                  label="Email: "
                  value={form.values.email}
                  error={getErrorForField(`email)}
                  onChange={ReForm.Helpers.handleDomFormChange(handleChange(`email))}
                />
                <Input
                  label="Password: "
                  value={form.values.password}
                  error={getErrorForField(`password)}
                  type_="password"
                  onChange={ReForm.Helpers.handleDomFormChange(handleChange(`password))}
                />
                <Input
                  label="Password (confirm): "
                  value={form.values.passwordConfirm}
                  error={getErrorForField(`passwordConfirm)}
                  type_="password"
                  onChange={ReForm.Helpers.handleDomFormChange(handleChange(`passwordConfirm))}
                />
                <Button disabled={self.state.isSubmitting} type_="submit"> {"SUBMIT" |> ReasonReact.string} </Button>
              </form>
            }
          </RegisterForm>
        }
      </RegisterMutation>
    </div>;
  },
};