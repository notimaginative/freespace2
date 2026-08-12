'use strict'

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//	General web UI stuff
//

// Alerts

const alertTypes = Object.freeze({
	info: 0,
	warn: 1,
	error: 2
})

const alerts = document.querySelector('#alerts')

function getAlertClass(value) {
	return Object.keys(alertTypes).find(key => alertTypes[key] === value)
}

function showAlert(msg, type = alertTypes.info, timeout = 5) {
	const box = document.querySelector('template[data-alert-box]').content.cloneNode(true).querySelector('.alert-box')

	const typeClass = getAlertClass(type)
	if (typeClass) box.classList.add(typeClass)

	const message = box.querySelector('[data-alert-msg]')
	message.textContent = msg

	const closeBtn = box.querySelector('button[data-close-btn]')
	closeBtn.addEventListener('click', () => box.remove())

	if (timeout) {
		setTimeout(() => closeBtn.click(), timeout*1000)
	}

	alerts.prepend(box)

	return box
}

// Navigation

const primaryNav = document.querySelector('nav.primary-nav')

// close all navs if we click outside of them
document.addEventListener('click', e => {
	if ( !primaryNav.contains(e.target) || e.target.classList.contains('primary-nav__front') ) {
		closeNavs()
	}
})

function closeNavs(ignorenav) {
	const openNavs = primaryNav.querySelectorAll('button[aria-controls][aria-expanded=true]')

	openNavs.forEach(nav => {
		if (nav === ignorenav) return

		toggleNav(nav)
	})
}

function toggleNav(elem) {
	const nav = document.getElementById(elem.getAttribute('aria-controls'))
	const navItems = nav.querySelectorAll('a, button, input, select')
	const visible = (nav.getAttribute('aria-hidden') === 'false')

	closeNavs(elem)

	nav.setAttribute('aria-hidden', visible)

	elem.setAttribute('aria-expanded', !visible)
	elem.setAttribute('aria-pressed', !visible)

	if (visible) {
		document.body.classList.remove('no-scroll')
	} else {
		document.body.classList.add('no-scroll')
	}

	const tabindex = visible ? '-1' : '0'

	for (const item of navItems) {
		item.setAttribute('tabindex', tabindex)
	}
}

// main content / status

function setConnectionStatus(connected = false) {
	const connStatus = document.querySelector('#conn-status')
	const content = document.querySelector('#main-content')

	if (connected) {
		connStatus.classList.add('hidden')
		content.classList.remove('hidden')
	} else {
		connStatus.textContent = 'Disconnected from server'

		content.classList.add('hidden')
		connStatus.classList.remove('hidden')
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Utilities / Generic functions
//

const Utils = {
	pad: function(value, count = 2) {
		return `${value}`.padStart(count, '0')
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// All of the websocket stuff below here ...
//

//
// msg type dispatcher
//
const msg_handler = Object.freeze({
	server: (msg) => Server.parse_msg(msg),
	netgame: (msg) => Netgame.parse_msg(msg),
	popup: (msg) => Popup.parse_msg(msg),
	reset_gui: (msg) => GUI.parse_msg(msg),
	mission: (msg) => Mission.parse_msg(msg),
	chat: (msg) => Chat.parse_msg(msg),
	player: (msg) => Players.parse_msg(msg),
	multilog: (msg) => Multilog.parse_msg(msg),
	server_info: (msg) => ServerInfo.parse_msg(msg)
})

//
// Server (websocket and config)
//
const Server = Object.freeze({
	elements: {
		panel: undefined
	},
	data: {
		ws: undefined,
		uptime_timer: undefined,
		msg_buffer: []
	},
	init: function() {
		this.elements.panel = document.getElementById('server')

		this.data.ws = new WebSocket((location.protocol === "https:" ? "wss://" : "ws://") + location.host, "standalone")

		this.data.ws.addEventListener('error', e => {
			showAlert('Error connecting to websocket!', alertTypes.error, 0)
		})

		this.data.ws.addEventListener('open', e => {
			setConnectionStatus(true)

			this.data.uptime_timer = setInterval(ServerInfo.calc_uptime, 1000*60)

			// if we have some messages waiting then send them
			for (const msg of this.data.msg_buffer) {
				this.send_msg(msg)
			}

			this.data.msg_buffer = []
		})

		this.data.ws.addEventListener('close', e => {
			setConnectionStatus(false)

			if (this.data.uptime_timer) {
				clearInterval(this.data.uptime_timer)
				this.data.uptime_timer = undefined
			}

			showAlert('Websocket closed', alertTypes.warn, 15)
		})

		this.data.ws.addEventListener('message', e => {
			try {
				const msg = JSON.parse(e.data)

				console.log(msg)

				for (const [key, value] of Object.entries(msg)) {
					if ( !msg_handler[key] ) {
						console.warn(`Unhandled message type '${key}' with value => `, value)
						continue
					}

					msg_handler[key](value)
				}
			} catch (err) {
				console.error(err)
				console.log(`Error parsing message from server! Message was: ${e.data}`)
			}
		})

		this.elements.panel.querySelector('[data-server-name]').addEventListener('change', () => this.set_name())
		this.elements.panel.querySelector('[data-server-password]').addEventListener('change', () => this.set_password())
		this.elements.panel.querySelector('[data-server-update_rate]').addEventListener('change', () => this.set_update_rate())
		this.elements.panel.querySelector('[data-server-max_players]').addEventListener('change', () => this.set_max_players())
		this.elements.panel.querySelector('[data-server-framecap]').addEventListener('change', () => this.set_framecap())
		this.elements.panel.querySelector('[data-server-pxo]').addEventListener('change', () => this.set_pxo())
		this.elements.panel.querySelector('[data-server-pxo_channel]').addEventListener('change', () => this.set_pxo_channel())
		this.elements.panel.querySelector('[data-server-voice]').addEventListener('change', () => this.set_voice())
	},
	isConnected: function() {
		return (this.data.ws && this.data.ws.readyState == 1)
	},
	send_msg: function(msg) {
		if (this.data.ws.readyState != 1 /* OPEN */) {
			this.data.msg_buffer.push(msg)

			// only keep 10 newest messages in buffer
			if (this.data.msg_buffer.length > 10) {
				this.data.msg_buffer.shift()
			}

			return
		}

		try {
			const jstr = JSON.stringify(msg)
			this.data.ws.send(jstr)
		} catch(e) {
			console.error(e)
		}
	},
	parse_msg: function(msg) {
		for (const [key, value] of Object.entries(msg)) {
			if ( !this.messages[key] ) {
				console.warn(`Unhandled server message type '${key}' => `, value)
				continue
			}

			this.messages[key](value)
		}
	},
	// --------------------------------------------------------------
	// messages from server
	//
	messages: Object.freeze({
		name: function(value) {
			if ( !value || value.length > 31) return

			const elem = Server.elements.panel.querySelector('[data-server-name]')

			elem.value = value
			elem.disabled = false
		},
		password: function(value) {
			if (value.length > 15) return

			const elem = Server.elements.panel.querySelector('[data-server-password]')

			elem.value = value
			elem.disabled = false
		},
		update_rate: function(value) {
			if (value < 0 || value > 3) return
	
			const elem = Server.elements.panel.querySelector('[data-server-update_rate]')
	
			elem.value = value
			elem.disabled = false
		},
		max_players: function(value) {
			// -1 sets default
			// max players is 12, but server counts as 1
			if ( !value || value != -1 || value >= 11) return
	
			const elem = Server.elements.panel.querySelector('[data-server-max_players]')
	
			elem.value = value
			elem.disabled = false
		},
		framecap: function(value) {
			if (value < 15 || value > 120) return

			const elem = Server.elements.panel.querySelector('[data-server-framecap]')

			elem.value = value
			elem.disabled = false
		},
		voice: function(value) {
			const elem = Server.elements.panel.querySelector('[data-server-voice]')

			elem.checked = value ? true : false
			elem.disabled = false
		},
		pxo: function(value) {
			const elem = Server.elements.panel.querySelector('[data-server-pxo]')

			elem.checked = value ? true : false
			elem.disabled = false
		},
		pxo_channel: function(value) {
			if (value.length > 31) return

			const elem = Server.elements.panel.querySelector('[data-server-pxo_channel]')

			elem.value = value
			elem.disabled = false
		}
	}),
	//
	// --------------------------------------------------------------

	// --------------------------------------------------------------
	// messags to server
	//
	set_name: function() {
		const name = this.elements.panel.querySelector('[data-server-name]').value.trim()
		this.send_msg({ "server": { "name": name } })
	},
	set_password: function() {
		const pass = this.elements.panel.querySelector('[data-server-password]').value.trim()
		this.send_msg({ "server": { "password": pass } })
	},
	set_voice: function() {
		const checked = this.elements.panel.querySelector('[data-server-voice]').checked
		this.send_msg({ "server": { "voice": checked } })
	},
	set_update_rate: function() {
		const selected = this.elements.panel.querySelector('[data-server-update_rate]').value
		this.send_msg({ "server": { "update_rate": parseInt(selected, 10) } })
	},
	set_max_players: function() {
		const m_players = parseInt(this.elements.panel.querySelector('[data-server-max_players]').value, 10)
		this.send_msg({ "server": { "max_players": m_players } })
	},
	set_pxo: function() {
		const checked = this.elements.panel.querySelector('[data-server-pxo]').checked
		this.send_msg({ "server": { "pxo": checked } })
	},
	set_pxo_channel: function() {
		let channel = this.elements.panel.querySelector('[data-server-pxo_channel]').value.trim()

		// make sure it's a proper irc channel type
		if (channel.length && !(channel === "global" || channel[0] == '#' || channel[0] == '+')) {
			channel = '#' + channel
			document.querySelector('#server [data-server-pxo_channel]').value = channel
		}

		this.send_msg({ "server": { "pxo_channel": channel } })
	},
	set_framecap: function() {
		const framecap = this.elements.panel.querySelector('[data-server-framecap]').value
		this.send_msg({ "server": { "framecap": parseInt(framecap, 10) } })
	},
	// re-validate missions with PXO server
	validate: function() {
		this.send_msg({ "server": { "validate": true } })
		closeNavs()
	},
	// shutdown standalone server
	shutdown: function() {
		this.send_msg({ "shutdown": true })
		closeNavs()
	},
	// reset standalone to main state (quits active mission!!!)
	reset_all: function() {
		this.send_msg({ "reset_all": true })
		closeNavs()
	},
	//
	// --------------------------------------------------------------
})

Server.init()

//
// GUI init/reset
//
const GUI = Object.freeze({
	parse_msg: function(msg) {
		this.reset()
	},
	reset: function() {
		Netgame.reset()
		Players.reset()
		Mission.reset()
		Chat.reset()
	}
})

//
// Popup
//
const Popup = Object.freeze({
	elements: {
		status_box: undefined
	},
	parse_msg: function(msg) {
		for (const [key, value] of Object.entries(msg)) {
			if ( !this.messages[key] ) {
				console.warn(`Unhandled popup message '${key}' => `, value)
				continue
			}

			this.messages[key](value)
		}
	},

	// --------------------------------------------------------------
	// messages from server
	//
	messages: Object.freeze({
		status: function(msg) {
			/*
			if ( !msg ) {
				if (Popup.elements.status_box) {
					setTimeout(() => {
						if (alerts.contains(Popup.elements.status_box)) {
							Popup.elements.status_box.remove()
						}

						Popup.elements.status_box = undefined
					}, 5000)
				}

				return
			} else if ( !msg['title'] ) {
				console.warn('Malformed popup status message => ', msg)
				return
			}

			let message = msg['title']

			if (msg['field1'] || msg['field2']) {
				message += " -> "

				if (msg['field1']) {
					message += msg['field1']

					if (msg['field2']) {
						message += ` ${msg['field2']}`
					}
				} else {
					message += msg['field2']
				}
			}

			if (Popup.elements.status_box && alerts.contains(Popup.elements.status_box)) {
				const elem = Popup.elements.status_box.querySelector('[data-alert-msg]')
				elem.textContent = message
			} else {
				Popup.elements.status_box = showAlert(message, alertTypes.info, 0)
			}
			*/
		},
		notice: function(msg) {
			if ( !msg['message'] ) {
				console.warn('Malformed popup notice message => ', msg)
				return	
			}

			showAlert(msg['message'], alertTypes.warn, 10)
		},
		alert: function(msg) {
			if ( !msg['message'] ) {
				console.warn('Malformed popup alert message => ', msg)
				return
			}

			alert(msg['message'])
		}
	})
})

//
// Server Info
//
const ServerInfo = Object.freeze({
	settings: {
		start_time: 0
	},
	parse_msg: function(msg) {
		for (const [key, value] of Object.entries(msg)) {
			if ( !this.messages[key] ) {
				console.warn(`Unhandled server_info message type '${key}' => `, value)
				continue
			}

			this.messages[key](value)
		}
	},
	calc_uptime: function() {
		const serverUptime = document.querySelector('#server-info [data-server-info-uptime]')

		if ( !ServerInfo.settings.start_time ) {
			serverUptime.textContent = '~'
			return
		}

		const seconds = Math.floor(Date.now() / 1000) - ServerInfo.settings.start_time
		const days = Math.floor(seconds / 86400)
		const hours = Math.floor(seconds / 3600) % 24
		const minutes = Utils.pad(Math.floor(seconds / 60) % 60)

		let uptime = `${hours}:${minutes}`

		if (days == 1) uptime = `${days} day, ${uptime}`
		else if (days > 1) uptime = `${days} days, ${uptime}`

		serverUptime.textContent = uptime
	},

	// --------------------------------------------------------------
	// messages from server
	//
	messages: Object.freeze({
		title: function(value) {
			document.title = value
		},
		build: function(value) {
			document.querySelector('#server-info [data-server-info-build]').textContent = value
		},
		multi_version: function(value) {
			document.querySelector('#server-info [data-server-info-multi_version]').textContent = value
		},
		state: function(value) {
			document.querySelector('#server-info [data-server-info-state]').textContent = value
		},
		address: function(value) {
			document.querySelector('#server-info [data-server-info-address]').textContent = value
		},
		start_time: function(value) {
			ServerInfo.settings.start_time = parseInt(value, 10)
			ServerInfo.calc_uptime()
		},
		realized_fps: function(value) {
			// console.log(`Realized fps: ${value}`)
		},
		host_connected: function(value) {
			// console.log(`Host connected: ${value}`)
		},
		num_players: function(value) {
			// console.log(`Num players: ${value}`)
		}
	})
	//
	// --------------------------------------------------------------
})

//
// Netgame
//
const Netgame = Object.freeze({
	parse_msg: function(msg) {
		for (const [key, value] of Object.entries(msg)) {
			if ( !this.messages[key] ) {
				console.warn(`Unhandled netgame message type '${key}' => `, value)
				continue
			}

			this.messages[key](value)
		}
	},
	reset: function () {
		const elems = document.querySelectorAll('#netgame td[data-netgame]')
		elems.forEach(elem => elem.textContent = '')
	},

	// --------------------------------------------------------------
	// messages from server
	//
	messages: Object.freeze({
		name: function(value) {
			document.querySelector('#netgame [data-netgame=name').textContent = value
		},
		mission_title: function(value) {
			document.querySelector('#netgame [data-netgame=mission_title').textContent = value
		},
		mission_name: function(value) {
			document.querySelector('#netgame [data-netgame=mission_name').textContent = value
		},
		campaign_name: function(value) {
			document.querySelector('#netgame [data-netgame=campaign_name').textContent = value
		},
		mode: function(value) {
			document.querySelector('#netgame [data-netgame=mode]').textContent = value
		},
		type: function(value) {
			document.querySelector('#netgame [data-netgame=type]').textContent = value
		},
		state: function(value) {
			document.querySelector('#netgame [data-netgame=state]').textContent = value
		},
		max_players: function(value) {
			document.querySelector('#netgame [data-netgame=max_players]').textContent = value
		},
		max_observers: function(value) {
			document.querySelector('#netgame [data-netgame=max_observers]').textContent = value
		},
		max_respawns: function(value) {
			document.querySelector('#netgame [data-netgame=max_respawns]').textContent = value
		}
	})
	//
	// --------------------------------------------------------------
})

//
// Players
//
const Players = Object.freeze({
	parse_msg: function(msg) {
		for (const [key, value] of Object.entries(msg)) {
			if ( !this.messages[key] ) {
				console.warn(`Unhandled player message type '${key}' => `, value)
				continue
			}

			this.messages[key](value)
		}
	},
	reset: function() {
		const tbody = document.querySelector('#players').tBodies[0]
		while (tbody.firstChild) tbody.firstChild.remove()

		this.reset_info()
	},
	reset_info: function() {
		const infoItems = document.querySelectorAll('#playerInfo td')
		infoItems.forEach(elem => elem.textContent = '')

		const allTimeStats = document.querySelectorAll('#playerStatsAllTime td')
		allTimeStats.forEach(elem => elem.textContent = '')

		const missionStats = document.querySelectorAll('#playerStatsMission td')
		missionStats.forEach(elem => elem.textContent = '')
	},

	// --------------------------------------------------------------
	// messages from server
	//
	messages: Object.freeze({
		add: function(player) {
			const tbody = document.querySelector('#players').tBodies[0]
			const row = tbody.insertRow()

			row.setAttribute('data-player-id', player.id)
			row.classList.add('actionable')

			row.insertCell().textContent = player.id
			row.insertCell().textContent = player.name
			row.insertCell().textContent = player.address
			row.insertCell().textContent = `${player.ping} ms`

			Array.from(row.cells).forEach(cell => {
				cell.classList.add('fs-300', 'text-200', 'ff-mono')
			})

			row.addEventListener('click', () => {
				Players.get_info(player.id)
			})
		},
		remove: function(player) {
			const row = document.querySelector(`#players [data-player-id='${player.id}']`)
			if (row) row.remove()
		},
		update: function(player) {
			const row = document.querySelector(`#players [data-player-id='${player.id}']`)
			if (row) row.cells[3].textContent = `${player.ping} ms`
		},
		info: function(player) {
			const Keys = Object.freeze({
				id: function(value) {
					document.querySelector('#playerInfo [data-info-id').textContent = value
				},
				name: function(value) {
					document.querySelector('#playerInfo [data-info-name').textContent = value
				},
				ping: function(value) {
					document.querySelector('#playerInfo [data-info-ping').textContent = value
				},
				address: function(value) {
					document.querySelector('#playerInfo [data-info-address').textContent = value
				},
				ship: function(value) {
					document.querySelector('#playerInfo [data-info-ship').textContent = value
				},
				rank: function(value) {
					document.querySelector('#playerInfo [data-info-rank').textContent = value
				},
				flight_time: function(value) {
					document.querySelector('#playerInfo [data-info-flight_time').textContent = value
				},
				missions_flown: function(value) {
					document.querySelector('#playerInfo [data-info-missions_flown').textContent = value
				},
				stats: function(value) {
					if (value['all-time']) {
						const stats = value['all-time']
						const root = document.getElementById('playerStatsAllTime')

						add_stats(stats, root)
					}

					if (value['mission']) {
						const stats = value['mission']
						const root = document.getElementById('playerStatsMission')

						add_stats(stats, root)
					}

					function add_stats(stats, statsElem) {
						if ( !Array.isArray(stats) || stats.length != 7) {
							console.warn('Invalid player stats element!')
							return
						}

						statsElem.querySelector('[data-stats-kills]').textContent = stats[0]
						statsElem.querySelector('[data-stats-friendly_kills]').textContent = stats[1]
						statsElem.querySelector('[data-stats-assists]').textContent = stats[2]
						statsElem.querySelector('[data-stats-primary_shots_fired]').textContent = stats[3]
						statsElem.querySelector('[data-stats-primary_hit_pct]').textContent = stats[4]
						statsElem.querySelector('[data-stats-secondary_shots_fired]').textContent = stats[5]
						statsElem.querySelector('[data-stats-secondary_hit_pct]').textContent = stats[6]
					}
				},
			})

			for (const [key, value] of Object.entries(player)) {
				if ( !Keys[key] ) {
					console.warn(`Unhandled player info message type '${key}' => `, value)
					continue
				}

				Keys[key](value)
			}
		}
	}),
	//
	// --------------------------------------------------------------

	// --------------------------------------------------------------
	// messages to server
	//
	get_info: function (player_id) {
		this.reset_info()
		Server.send_msg({ "player": { "info": player_id } })
	},
	kick: function(player_id) {
		Server.send_msg({ "player": { "kick": player_id } })
	}
	//
	// --------------------------------------------------------------
})

//
// Mission
//
const Mission = Object.freeze({
	data: {
		time_sync: false,
		mission_time: -1,
		mission_time_timer: undefined,
	},
	parse_msg: function(msg) {
		for (const [key, value] of Object.entries(msg)) {
			if ( !this.messages[key] ) {
				console.warn(`Unhandled mission message type '${key}' => `, value)
				continue
			}

			this.messages[key](value)
		}
	},
	reset: function () {
		this.data.mission_time = -1

		if (this.data.mission_time_timer) {
			clearInterval(this.data.mission_time_timer)
			this.data.mission_time_timer = undefined
		}

		const missionTime = document.querySelector('#mission [data-mission-time]')
		missionTime.textContent = ''

		this.reset_goals()
	},
	reset_goals: function () {
		const goals = ['#primaryGoals', '#secondaryGoals', '#bonusGoals']

		for (const item of goals) {
			const elem = document.querySelector(item)
			if (!elem) continue

			elem.classList.add('hidden')
			while (elem.tBodies[0].firstChild) elem.tBodies[0].firstChild.remove()
		}
	},
	update_time: function () {
		const missionTime = document.querySelector('#mission [data-mission-time]')

		if (this.data.mission_time == -1) {
			missionTime.textContent = ''

			if (this.data.mission_time_timer) {
				clearInterval(this.data.mission_time_timer)
				this.data.mission_time_timer = undefined
			}

			this.data.time_sync = false

			return
		}

		const hours = Math.floor(this.data.mission_time / 3600)
		const minutes = Utils.pad(Math.floor(this.data.mission_time / 60) % 60)
		const seconds = Utils.pad(this.data.mission_time % 60)

		let time = `${minutes}:${seconds}`

		if (hours > 0) {
			time = `${hours}:${time}`
		}

		missionTime.textContent = time

		this.data.time_sync = false
		// the time message is rate limited so we use an interval to keep it in
		// sync between updates
		if ( !this.data.mission_time_timer ) {
			this.data.mission_time_timer = setInterval(() => {
				// don't try to update while we're doing a server sync
				if (Mission.data.time_sync) return

				Mission.data.mission_time += 1
				Mission.update_time()
			}, 1000)
		}
	},

	// --------------------------------------------------------------
	// messages from server
	//
	messages: Object.freeze({
		time: function (mtime) {
			Mission.data.time_sync = true
			Mission.data.mission_time = Math.floor(mtime)
			Mission.update_time()
		},
		goals: function(goals) {
			const goalTypes = ['primary', 'secondary', 'bonus']
			const statusNames = ['failed', 'complete', 'incomplete']

			Mission.reset_goals()

			for (const type of goalTypes) {
				if (!Array.isArray(goals[type])) continue
				if (!goals[type].length) continue

				try {
					const table = document.querySelector(`#${type}Goals`)

					goals[type].forEach(goal => {
						const row = table.tBodies[0].insertRow()

						const goalName = document.createElement('th')
						goalName.setAttribute('scope', 'row')
						goalName.classList.add('goal_name')
						goalName.textContent = goal.name

						row.appendChild(goalName)
						row.insertCell().textContent = statusNames[goal.status]

						row.cells[1].setAttribute('data-goal-status', `${statusNames[goal.status]}`)
					})

					table.classList.remove('hidden')
				} catch(e) {}
			}
		},
		reset_goals: function () {
			Mission.reset_goals()
		}
	})
	//
	// --------------------------------------------------------------
})

//
// Multi log
//
const Multilog = Object.freeze({
	elements: {
		panel: undefined
	},
	settings: {
		MAX_LINES: 100,
		enabled: undefined,
		clear_existing: false
	},
	init: function() {
		this.elements.panel = document.getElementById('multilog')

		const multilog_enable = localStorage.getItem('multilogEnabled')

		if (multilog_enable != null) {
			this.enable(multilog_enable == 'true')
		} else {
			this.enable(false)
		}

		this.elements.panel.querySelector('[data-multilog-enable]').addEventListener('change', e => {
			this.enable(e.target.checked)
		})
	},
	parse_msg: function(msg) {
		const output = this.elements.panel.querySelector('[data-multilog]')

		if (this.settings.clear_existing) {
			while (output.lastChild) {
				output.lastChild.remove()
			}

			this.settings.clear_existing = false
		}

		const lineTemplate = document.querySelector('template[data-template=output_line]')
		const line = lineTemplate.content.cloneNode(true)

		if (msg.length > 15 && msg[14] === '~') {
			line.querySelector('.message_id').textContent = msg.substring(0, 15).trim()
			line.querySelector('.message').textContent = msg.substring(15).trim()
		} else {
			line.querySelector('.message').textContent = msg
		}

		output.appendChild(line)

		// prevent list from getting too large (NOTE: there are 2 elements per line!!)
		if (output.children.length > (this.settings.MAX_LINES*2)) {
			output.firstChild.remove()
			output.firstChild.remove()
		}

		output.scrollTop = output.lastElementChild.offsetTop
	},
	enable: function(value = false) {
		if (value === this.settings.enabled) return

		this.settings.enabled = value ? true : false

		this.elements.panel.querySelector('[data-multilog-enable]').checked = value

		try {
			localStorage.setItem('multilogEnabled', this.settings.enabled)
		} catch(e) { }

		// if switching to enabled then make sure to clear exisitng entries on next parse
		if (this.settings.enabled) {
			this.settings.clear_existing = true
		}

		Server.send_msg({ "multilog": this.settings.enabled })
	}
})

Multilog.init()

//
// Chat
//
const Chat = Object.freeze({
	elements: {
		panel: undefined
	},
	settings: {
		MAX_LINES: 100,
	},
	init: function() {
		this.elements.panel = document.getElementById('chatbox')

		document.getElementById('chatMessage').addEventListener('keydown', e => {
			if (e.key === 'Enter') {
				const msg = e.target.value.trim()

				if (msg.length) {
					Server.send_msg({ "chat": msg })
				}

				e.target.value = ''
			}
		})
	},
	reset: function() {
		if ( !this.elements.panel ) return

		while (this.elements.panel.lastChild) {
			this.elements.panel.lastChild.remove()
		}
	},
	parse_msg: function(msg) {
		const lineTemplate = document.querySelector('template[data-template=output_line]')
		const line = lineTemplate.content.cloneNode(true)

		line.querySelector('.message_id').textContent = msg.id || ''
		line.querySelector('.message').textContent = msg.message

		this.elements.panel.appendChild(line)

		// prevent list from getting too large (NOTE: there are 2 elements per line!!)
		if (this.elements.panel.children.length > (this.settings.MAX_LINES*2)) {
			this.elements.panel.firstChild.remove()
			this.elements.panel.firstChild.remove()
		}

		this.elements.panel.scrollTop = this.elements.panel.lastElementChild.offsetTop
	}
})

Chat.init()
