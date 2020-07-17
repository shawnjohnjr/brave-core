import { createReducer } from 'redux-act'
import * as Actions from '../../actions/today_actions'

const defaultState: NewTab.BraveTodayState = {
  isLoaded: false,
  isLoading: true
}

const reducer = createReducer<NewTab.BraveTodayState>({}, defaultState)

export default reducer

reducer.on(Actions.init, (state, payload) => ({
  ...state,
  isLoading: true
}))

