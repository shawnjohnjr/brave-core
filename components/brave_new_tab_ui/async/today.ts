import * as Background from '../../common/Background'
import AsyncActionHandler from '../../common/AsyncActionHandler'
// import * as Actions from '../actions/today_actions'
import { init } from '../actions/new_tab_actions'
// import '../../definitions/newTab.d'

const handler = new AsyncActionHandler()

handler.on<void>(init.getType(), async ({ getState, dispatch}, action) => {
  console.log('waiting for today feed', Background.MessageType.getFeed, Background.MessageType.indicatingOpen)
  const feed = await Background.send(Background.MessageType.getFeed)
  console.log('got feed', feed)
})


export default handler.middleware